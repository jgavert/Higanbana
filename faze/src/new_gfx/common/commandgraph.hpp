#pragma once
#include "commandlist.hpp"
#include "core/src/datastructures/proxy.hpp"
#include "core/src/system/memview.hpp"
#include "core/src/global_debug.hpp"

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
    }

    void clearRT(TextureRTV& rtv, vec4 color)
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