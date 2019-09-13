#pragma once
#include "higanbana/graphics/common/commandlist.hpp"
#include "higanbana/graphics/common/semaphore.hpp"
#include "higanbana/graphics/common/binding.hpp"
#include "higanbana/graphics/common/swapchain.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/desc/timing.hpp"
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/global_debug.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <higanbana/core/entity/bitfield.hpp>

#include <higanbana/core/math/utils.hpp>

#include <string>
#include "higanbana/graphics/common/readback.hpp"
#include <memory>

namespace higanbana
{
  class CommandGraphNode
  {
  private:
    CommandList list;
    std::string name;
    friend struct backend::DeviceGroupData;
    friend struct CommandGraph;
    QueueType type;
    int gpuId;
    std::shared_ptr<backend::SemaphoreImpl> acquireSemaphore;
    bool preparesPresent = false;

    DynamicBitfield m_referencedBuffers;
    DynamicBitfield m_referencedTextures;
    vector<ReadbackPromise> m_readbackPromises;
    GraphNodeTiming timing;

    const DynamicBitfield& refBuf() const
    {
      return m_referencedBuffers;
    }
    const DynamicBitfield& refTex() const
    {
      return m_referencedTextures;
    }

    template <typename ViewType>
    void addViewBuf(ViewType type)
    {
      auto h = type.handle();
      m_referencedBuffers.setBit(h.resource.id);
    }

    template <typename ViewType>
    void addViewTex(ViewType type)
    {
      auto h = type.handle();
      m_referencedTextures.setBit(h.resource.id);
    }

    void addMemview(MemView<ViewResourceHandle> views)
    {
      for (auto&& view : views)
      {
        switch (view.type)
        {
          case ViewResourceType::BufferSRV:
          case ViewResourceType::BufferUAV:
          case ViewResourceType::BufferIBV:
          {
            m_referencedBuffers.setBit(view.resource.id);
            break;
          }
          case ViewResourceType::TextureSRV:
          case ViewResourceType::TextureUAV:
          case ViewResourceType::TextureRTV:
          case ViewResourceType::TextureDSV:
          {
            m_referencedTextures.setBit(view.resource.id);
            break;
          }
          default:
            break;
        }
      }
    }
  public:
    CommandGraphNode() {}
    CommandGraphNode(std::string name, QueueType type, int gpuId)
      : name(name)
      , type(type)
      , gpuId(gpuId)
    {
      list.renderTask(name);
      timing.nodeName = name;
      timing.cpuTime.start();
    }

    void acquirePresentableImage(Swapchain& swapchain)
    {
      acquireSemaphore = swapchain.impl()->acquireSemaphore();
    }

    void clearRT(TextureRTV& rtv, float4 color)
    {
      addViewTex(rtv);
      list.clearRT(rtv, color);
    }

    void prepareForPresent(TextureRTV& rtv)
    {
      list.prepareForPresent(rtv);
      preparesPresent = true;
    }

    void renderpass(Renderpass& pass, TextureRTV& rtv)
    {
      addViewTex(rtv);
      list.renderpass(pass, {rtv}, {});
    }

    void renderpass(Renderpass& pass, TextureRTV& rtv, TextureDSV& dsv)
    {
      addViewTex(rtv);
      addViewTex(dsv);
      list.renderpass(pass, {rtv}, dsv);
    }

    void renderpass(Renderpass& pass, TextureDSV& dsv)
    {
      addViewTex(dsv);
      list.renderpass(pass, {}, dsv);
    }

    void endRenderpass()
    {
      list.renderpassEnd();
    }

    // bindings

    Binding bind(GraphicsPipeline& pipeline)
    {
      list.bindPipeline(pipeline);

      return Binding(pipeline);
    }

    Binding bind(ComputePipeline& pipeline)
    {
      list.bindPipeline(pipeline);

      return Binding(pipeline);
    }

    void setScissor(int2 tl, int2 br)
    {
      list.setScissorRect(tl, br);
    }

    // draws/dispatches

    void draw(
      Binding& binding,
      unsigned vertexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned startVertex = 0,
      unsigned startInstance = 0)
    {
      addMemview(binding.bResources());
      list.bindGraphicsResources(binding.bResources(), binding.bConstants());
      HIGAN_ASSERT(vertexCountPerInstance > 0 && instanceCount > 0, "Index/instance count was 0, nothing would be drawn. draw %d %d %d %d", vertexCountPerInstance, instanceCount, startVertex, startInstance);
      list.draw(vertexCountPerInstance, instanceCount, startVertex, startInstance);
    }

    void drawIndexed(
      Binding& binding,
      DynamicBufferView view,
      unsigned IndexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned StartIndexLocation = 0,
      int BaseVertexLocation = 0,
      unsigned StartInstanceLocation = 0)
    {
      addMemview(binding.bResources());
      list.bindGraphicsResources(binding.bResources(), binding.bConstants());
      HIGAN_ASSERT(IndexCountPerInstance > 0 && instanceCount > 0, "Index/instance count was 0, nothing would be drawn. drawIndexed %d %d %d %d %d", IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
      list.drawDynamicIndexed(view, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void drawIndexed(
      Binding& binding,
      BufferIBV view,
      unsigned IndexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned StartIndexLocation = 0,
      int BaseVertexLocation = 0,
      unsigned StartInstanceLocation = 0)
    {
      addMemview(binding.bResources());
      list.bindGraphicsResources(binding.bResources(), binding.bConstants());
      HIGAN_ASSERT(IndexCountPerInstance > 0 && instanceCount > 0, "Index/instance count was 0, nothing would be drawn. drawIndexed %d %d %d %d %d", IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
      list.drawIndexed(view, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void dispatchThreads(
      Binding& binding, uint3 groups)
    {
      addMemview(binding.bResources());
      list.bindComputeResources(binding.bResources(), binding.bConstants());
      unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(groups.x), static_cast<uint64_t>(binding.baseGroups().x)));
      unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(groups.y), static_cast<uint64_t>(binding.baseGroups().y)));
      unsigned z = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(groups.z), static_cast<uint64_t>(binding.baseGroups().z)));
      HIGAN_ASSERT(x*y*z > 0, "One of the parameters was 0, no threadgroups would be launched. dispatch %d %d %d", x, y, z);
      list.dispatch(uint3(x,y,z));
    }

    void dispatch(
      Binding& binding, uint3 groups)
    {
      addMemview(binding.bResources());
      list.bindComputeResources(binding.bResources(), binding.bConstants());
      HIGAN_ASSERT(groups.x*groups.y*groups.z > 0, "One of the parameters was 0, no threadgroups would be launched. dispatch %d %d %d", groups.x, groups.y, groups.z);
      list.dispatch(groups);
    }

    void copy(Buffer target, int64_t elementOffset, Texture source, Subresource sub)
    {
      m_referencedBuffers.setBit(target.handle().id);
      auto mipDim = calculateMipDim(source.desc().size(), sub.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list.copy(target, elementOffset, source, sub, srcBox);
    }

    void copy(Texture target, Texture source)
    {
      m_referencedTextures.setBit(target.handle().id);
      m_referencedTextures.setBit(source.handle().id);
      HIGAN_ASSERT(false, "Not implemented.");
      //auto lol = source.dependency();
      /*
      SubresourceRange range{
        static_cast<int16_t>(lol.mip()),
        static_cast<int16_t>(lol.mipLevels()),
        static_cast<int16_t>(lol.slice()),
        static_cast<int16_t>(lol.arraySize()) };
      list.copy(target, source, range);
      */
    }

    void copy(Buffer target, Buffer source)
    {
      m_referencedBuffers.setBit(target.handle().id);
      m_referencedBuffers.setBit(source.handle().id);
      list.copy(target, 0, source, 0, source.desc().desc.width);
    }

    void copy(Buffer target, uint targetElementOffset, Buffer source, uint sourceElementOffset = 0, int sourceElementsToCopy = -1)
    {
      if (sourceElementsToCopy == -1)
      {
        sourceElementsToCopy = source.desc().desc.width;
      }
      m_referencedBuffers.setBit(target.handle().id);
      m_referencedBuffers.setBit(source.handle().id);
      list.copy(target, targetElementOffset, source, sourceElementOffset, sourceElementsToCopy);
    }

    void copy(Buffer target, DynamicBufferView source)
    {
      auto elements = source.logicalSize() / source.elementSize();
      m_referencedBuffers.setBit(target.handle().id);
      list.copy(target, 0, source, 0, elements);
    }

    void copy(Buffer target, uint targetElementOffset, DynamicBufferView source, uint sourceElementOffset = 0, int sourceElementsToCopy = -1)
    {
      if (sourceElementsToCopy == -1)
      {
        sourceElementsToCopy = source.logicalSize() / source.elementSize();
      }
      m_referencedBuffers.setBit(target.handle().id);
      list.copy(target, targetElementOffset, source, sourceElementOffset, sourceElementsToCopy);
    }

    void copy(Texture target, Buffer source, Subresource sub)
    {
      m_referencedBuffers.setBit(target.handle().id);
      m_referencedBuffers.setBit(source.handle().id);
      HIGAN_ASSERT(false, "Not implemented");
    }

    void copy(Texture target, DynamicBufferView source, Subresource sub)
    {
      m_referencedTextures.setBit(target.handle().id);
      list.updateTexture(target, source, sub.mipLevel, sub.arraySlice);
    }

    ReadbackFuture readback(Texture tex, Subresource resource)
    {
      auto promise = ReadbackPromise({nullptr, std::make_shared<std::promise<ReadbackData>>()});
      m_readbackPromises.push_back(promise);
      m_referencedTextures.setBit(tex.handle().id);
      auto mipDim = calculateMipDim(tex.desc().size(), resource.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list.readback(tex, resource, srcBox);
      return promise.future();
    }

    ReadbackFuture readback(Buffer buffer, int offset = -1, int size = -1)
    {
      auto promise = ReadbackPromise({nullptr, std::make_shared<std::promise<ReadbackData>>()});
      m_readbackPromises.push_back(promise);
      m_referencedBuffers.setBit(buffer.handle().id);

      if (offset == -1)
      {
        offset = 0;
      }

      if (size == -1)
      {
        size = buffer.desc().desc.width;
      }

      list.readback(buffer, offset, size);
      return promise.future();
    }

    ReadbackFuture readback(Buffer buffer, unsigned startElement, unsigned size)
    {
      auto promise = ReadbackPromise({nullptr, std::make_shared<std::promise<ReadbackData>>()});
      m_readbackPromises.push_back(promise);
      m_referencedBuffers.setBit(buffer.handle().id);
      list.readback(buffer, startElement, size);
      return promise.future();
    }
  };

  class CommandGraph
  {
    SeqNum m_sequence;
    SubmitTiming m_timing;
    std::shared_ptr<vector<CommandGraphNode>> m_nodes;
    friend struct backend::DeviceGroupData;
  public:
    CommandGraph(SeqNum seq)
      : m_sequence(seq)
      , m_nodes{ std::make_shared<vector<CommandGraphNode>>() }
    {
      m_timing.timeBeforeSubmit.start();
    }

    CommandGraphNode createPass(std::string name, QueueType type = QueueType::Graphics, int gpu = 0)
    {
      return CommandGraphNode(name, type, gpu);
    }

    void addPass(CommandGraphNode&& node)
    {
      node.timing.cpuTime.stop();
      m_nodes->emplace_back(std::move(node));
    }
  };
}