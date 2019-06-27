#pragma once
#include "graphics/common/commandlist.hpp"
#include "core/datastructures/proxy.hpp"
#include "core/system/memview.hpp"
#include "core/global_debug.hpp"
#include "graphics/common/semaphore.hpp"
#include "graphics/common/binding.hpp"
#include "graphics/common/swapchain.hpp"
#include "graphics/common/handle.hpp"
#include <core/system/SequenceTracker.hpp>
#include <core/entity/bitfield.hpp>

#include <core/math/utils.hpp>

#include <string>

namespace faze
{
  class CommandGraphNode
  {
  public:
    enum class NodeType
    {
      Graphics,
      Compute,
      DMA
    };
  private:
    CommandList list;
    std::string name;
    friend struct backend::DeviceGroupData;
    NodeType type;
    int gpuId;
    std::shared_ptr<backend::SemaphoreImpl> acquireSemaphore;
    bool preparesPresent = false;

    DynamicBitfield2 m_referencedBuffers;
    DynamicBitfield2 m_referencedTextures;

    const DynamicBitfield2& refBuf() const
    {
      return m_referencedBuffers;
    }
    const DynamicBitfield2& refTex() const
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
    CommandGraphNode(std::string name, NodeType type, int gpuId)
      : name(name)
      , type(type)
      , gpuId(gpuId)
    {
      list.renderTask(name);
    }

    void resetState(Texture& tex)
    {
      // This was probably used for resetting state for queue ownership transfers.
      //needSpecialAttention.insert(tex.dependency());
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

    // starting another subpass will end the last one.
    // collecting the dependencies here.
    // refactoring later if too cubersome.
    void renderpass(Renderpass& pass, TextureRTV& rtv)
    {
      addViewTex(rtv);
      list.renderpass(pass, {rtv}, {});
    }
    int renderpass(Renderpass& pass, TextureRTV& rtv, TextureDSV& dsv)
    {
      addViewTex(rtv);
      addViewTex(dsv);
      list.renderpass(pass, {rtv}, {dsv});
    }
    int renderpass(Renderpass& pass, TextureDSV& dsv)
    {
      addViewTex(dsv);
      list.renderpass(pass, {}, {dsv});
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
      list.dispatch(uint3(x,y,z));
    }

    void dispatch(
      Binding& binding, uint3 groups)
    {
      addMemview(binding.bResources());
      list.bindComputeResources(binding.bResources(), binding.bConstants());
      list.dispatch(groups);
    }

    void copy(Buffer target, int64_t elementOffset, Texture source, Subresource sub)
    {
      //addViewBuf(target)
      m_referencedBuffers.setBit(target.handle().id);
      auto mipDim = calculateMipDim(source.desc().size(), sub.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list.copy(target, elementOffset, source, sub, srcBox);
    }

    void copy(Texture target, Texture source)
    {
      m_referencedTextures.setBit(target.handle().id);
      m_referencedTextures.setBit(source.handle().id);
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
      list.copy(target, source);
    }

    void copy(Buffer target, DynamicBufferView source)
    {
      m_referencedBuffers.setBit(target.handle().id);
      list.copy(target, source);
    }

    void readback(Texture tex, Subresource resource, std::function<void(SubresourceData)> func)
    {
      m_referencedTextures.setBit(tex.handle().id);
      auto mipDim = calculateMipDim(tex.desc().size(), resource.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list.readback(tex, resource, srcBox, func);
    }

    void readback(Buffer buffer, std::function<void(MemView<uint8_t>)> func)
    {
      m_referencedBuffers.setBit(buffer.handle().id);
      list.readback(buffer, 0, buffer.desc().desc.width, func);
    }

    void readback(Buffer buffer, unsigned startElement, unsigned size, std::function<void(MemView<uint8_t>)> func)
    {
      m_referencedBuffers.setBit(buffer.handle().id);
      list.readback(buffer, startElement, size, func);
    }

    void queryCounters(std::function<void(MemView<std::pair<std::string, double>>)> func)
    {
      list.queryCounters(func);
    }
  };

  class CommandGraph
  {
    SeqNum m_sequence;
    std::shared_ptr<vector<CommandGraphNode>> m_nodes;
    friend struct backend::DeviceGroupData;
  public:
    CommandGraph(SeqNum seq)
      : m_sequence(seq)
      , m_nodes{ std::make_shared<vector<CommandGraphNode>>() }
    {}

    CommandGraphNode createPass(std::string name, CommandGraphNode::NodeType type = CommandGraphNode::NodeType::Graphics, int gpu = 0)
    {
      return CommandGraphNode(name, type, gpu);
    }

    void addPass(CommandGraphNode&& node)
    {
      m_nodes->emplace_back(std::move(node));
    }

    CommandGraphNode& createPass2(std::string name, CommandGraphNode::NodeType type = CommandGraphNode::NodeType::Graphics, int gpu = 0)
    {
      m_nodes->emplace_back(name, type, gpu);
      return m_nodes->back();
    }
  };
}