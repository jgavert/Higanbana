#pragma once
#include "commandlist.hpp"
#include "core/src/datastructures/proxy.hpp"
#include "core/src/system/memview.hpp"
#include "core/src/global_debug.hpp"
#include "binding.hpp"

#include <string>

namespace faze
{
  class CommandGraphNode
  {
    CommandList list;
    std::string name;
    friend struct backend::DeviceData;
    int subpassIndex = 0;
  public:
    CommandGraphNode() {}
    CommandGraphNode(std::string name)
      : name(name)
    {
      list.renderTask(name);
    }

    void clearRT(TextureRTV& rtv, float4 color)
    {
      list.clearRT(rtv, color);
    }

    void prepareForPresent(TextureRTV& rtv)
    {
      list.prepareForPresent(rtv);
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

    template <typename Shader>
    Binding<Shader> bind(GraphicsPipeline& pipeline)
    {
      list.bindPipeline(pipeline);

      return Binding<Shader>();
    }

    template <typename Shader>
    Binding<Shader> bind(ComputePipeline& pipeline)
    {
      list.bindPipeline(pipeline);

      return Binding<Shader>();
    }

    void setScissor(int2 tl, int2 br)
    {
      list.setScissorRect(tl, br);
    }

    // draws/dispatches

    template <typename Shader>
    void draw(
      Binding<Shader>& binding,
      unsigned vertexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned startVertex = 0,
      unsigned startInstance = 0)
    {
      list.bindGraphicsResources(binding.bResources(), binding.bConstants(), binding.bSrvs(), binding.bUavs());
      list.draw(vertexCountPerInstance, instanceCount, startVertex, startInstance);
    }

    template <typename Shader>
    void drawIndexed(
      Binding<Shader>& binding,
      DynamicBufferView view,
      unsigned IndexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned StartIndexLocation = 0,
      int BaseVertexLocation = 0,
      unsigned StartInstanceLocation = 0)
    {
      list.bindGraphicsResources(binding.bResources(), binding.bConstants(), binding.bSrvs(), binding.bUavs());
      list.drawDynamicIndexed(view, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    template <typename Shader>
    void drawIndexed(
      Binding<Shader>& binding,
      BufferIBV view,
      unsigned IndexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned StartIndexLocation = 0,
      int BaseVertexLocation = 0,
      unsigned StartInstanceLocation = 0)
    {
      list.bindGraphicsResources(binding.bResources(), binding.bConstants(), binding.bSrvs(), binding.bUavs());
      list.drawIndexed(view, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    template <typename Shader>
    void dispatch(
      Binding<Shader>& binding, uint3 groups)
    {
      list.bindComputeResources(binding.bResources(), binding.bConstants(), binding.bSrvs(), binding.bUavs());
      list.dispatch(groups);
    }

    void copy(Buffer target, Buffer source)
    {
      list.copy(target, source);
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

    CommandGraphNode createPass(std::string name)
    {
      return CommandGraphNode(name);
    }

    void addPass(CommandGraphNode&& node)
    {
      m_nodes->emplace_back(std::move(node));
    }

    CommandGraphNode& createPass2(std::string name)
    {
      m_nodes->emplace_back(name);
      return m_nodes->back();
    }
  };
}