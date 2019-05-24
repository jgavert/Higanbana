#pragma once
#include "graphics/common/commandlist.hpp"
#include "core/datastructures/proxy.hpp"
#include "core/system/memview.hpp"
#include "core/global_debug.hpp"
#include "graphics/common/semaphore.hpp"
#include "graphics/common/binding.hpp"
#include "graphics/common/swapchain.hpp"

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
    friend struct backend::DeviceData;
    int subpassIndex = 0;
    NodeType type;
    std::shared_ptr<backend::SemaphoreImpl> acquireSemaphore;
    bool preparesPresent = false;

    unordered_set<backend::TrackedState> needSpecialAttention;

    unordered_set<backend::TrackedState>& needsResources()
    {
      return needSpecialAttention;
    }

    vector<std::shared_ptr<backend::SemaphoreImpl>> m_wait;
    vector<std::shared_ptr<backend::SemaphoreImpl>> m_signal;
  public:
    CommandGraphNode() {}
    CommandGraphNode(std::string name, NodeType type)
      : name(name)
      , type(type)
    {
      list.renderTask(name);
    }

    CommandGraphNode(std::string name, NodeType type, MemView<GpuSemaphore> wait, MemView<GpuSemaphore> signal)
      : name(name)
      , type(type)
    {
      list.renderTask(name);

      for (auto&& it : wait)
      {
        m_wait.push_back(it.native());
      }

      for (auto&& it : signal)
      {
        m_signal.push_back(it.native());
      }
    }

    void resetState(Texture& tex)
    {
      needSpecialAttention.insert(tex.dependency());
    }

    void acquirePresentableImage(Swapchain& swapchain)
    {
      acquireSemaphore = swapchain.impl()->acquireSemaphore();
    }

    void clearRT(TextureRTV& rtv, float4 color)
    {
      list.clearRT(rtv, color);
    }

    void prepareForPresent(TextureRTV& rtv)
    {
      list.prepareForPresent(rtv);
      preparesPresent = true;
    }

    void renderpass(Renderpass& pass)
    {
      subpassIndex = 0;
      list.renderpass(pass);
    }
    void endRenderpass()
    {
      list.renderpassEnd();
    }

    // starting another subpass will end the last one.
    // collecting the dependencies here.
    // refactoring later if too cubersome.
    int subpass(TextureRTV& rtv, MemView<int> deps = {})
    {
      list.subpass(deps, { rtv }, {  });
      return subpassIndex++;
    }
    int subpass(TextureRTV& rtv, TextureDSV& dsv, MemView<int> deps = {})
    {
      list.subpass(deps, { rtv }, { dsv });
      return subpassIndex++;
    }
    int subpass(TextureDSV& dsv, MemView<int> deps = {})
    {
      list.subpass(deps, {}, { dsv });
      return subpassIndex++;
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
      list.bindGraphicsResources(binding.bResources(), binding.bConstants(), binding.bViews());
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
      list.bindGraphicsResources(binding.bResources(), binding.bConstants(), binding.bViews());
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
      list.bindGraphicsResources(binding.bResources(), binding.bConstants(), binding.bViews());
      list.drawIndexed(view, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void dispatch(
      Binding& binding, uint3 groups)
    {
      list.bindComputeResources(binding.bResources(), binding.bConstants(), binding.bViews());
      list.dispatch(groups);
    }

    void copy(Buffer target, int64_t elementOffset, Texture source, Subresource sub)
    {
      auto lol = source.dependency();
      auto mipDim = calculateMipDim(source.desc().size(), sub.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list.copy(target, elementOffset, source, sub, srcBox);
    }

    void copy(Texture target, Texture source)
    {
      auto lol = source.dependency();
      SubresourceRange range{
        static_cast<int16_t>(lol.mip()),
        static_cast<int16_t>(lol.mipLevels()),
        static_cast<int16_t>(lol.slice()),
        static_cast<int16_t>(lol.arraySize()) };
      list.copy(target, source, range);
    }

    void copy(Buffer target, Buffer source)
    {
      list.copy(target, source);
    }

    void copy(Buffer target, DynamicBufferView source)
    {
      list.copy(target, source);
    }

    void readback(Texture tex, Subresource resource, std::function<void(SubresourceData)> func)
    {
      auto lol = tex.dependency();
      auto mipDim = calculateMipDim(tex.desc().size(), resource.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list.readback(tex, resource, srcBox, func);
    }

    void readback(Buffer buffer, std::function<void(MemView<uint8_t>)> func)
    {
      list.readback(buffer, 0, buffer.desc().desc.width, func);
    }

    void readback(Buffer buffer, unsigned startElement, unsigned size, std::function<void(MemView<uint8_t>)> func)
    {
      list.readback(buffer, startElement, size, func);
    }

    void queryCounters(std::function<void(MemView<std::pair<std::string, double>>)> func)
    {
      list.queryCounters(func);
    }
  };

  class CommandGraph
  {
    std::shared_ptr<vector<CommandGraphNode>> m_nodes;
    friend struct backend::DeviceData;
  public:
    CommandGraph()
      : m_nodes{ std::make_shared<vector<CommandGraphNode>>() }
    {}

    CommandGraphNode createPass(std::string name, CommandGraphNode::NodeType type = CommandGraphNode::NodeType::Graphics)
    {
      return CommandGraphNode(name, type);
    }

    CommandGraphNode createPass(std::string name, MemView<GpuSemaphore> wait, MemView<GpuSemaphore> signal, CommandGraphNode::NodeType type = CommandGraphNode::NodeType::Graphics)
    {
      return CommandGraphNode(name, type, wait, signal);
    }

    void addPass(CommandGraphNode&& node)
    {
      m_nodes->emplace_back(std::move(node));
    }

    CommandGraphNode& createPass2(std::string name, CommandGraphNode::NodeType type = CommandGraphNode::NodeType::Graphics)
    {
      m_nodes->emplace_back(name, type);
      return m_nodes->back();
    }

    CommandGraphNode& createPass2(std::string name, MemView<GpuSemaphore> wait, MemView<GpuSemaphore> signal, CommandGraphNode::NodeType type = CommandGraphNode::NodeType::Graphics)
    {
      m_nodes->emplace_back(name, type, wait, signal);
      return m_nodes->back();
    }
  };
}