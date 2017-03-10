#pragma once
#include "commandlist.hpp"
#include "core/src/datastructures/proxy.hpp"
#include "core/src/global_debug.hpp"

#include <string>

namespace faze
{
  class CommandGraphNode
  {
    CommandList list;
    std::string name;
    friend struct backend::DeviceData;
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
  };

  class CommandGraph
  {
    std::shared_ptr<vector<CommandGraphNode>> m_nodes;
    friend struct backend::DeviceData;
  public:
    CommandGraph()
      : m_nodes{std::make_shared<vector<CommandGraphNode>>()}
    {}

    CommandGraphNode createPass(std::string name)
    {
      return CommandGraphNode(name);
    }

    void addPass(CommandGraphNode node)
    {
      m_nodes->emplace_back(std::move(node));
    }
  };
}
