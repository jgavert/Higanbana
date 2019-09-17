#pragma once

#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/desc/shader_input_descriptor.hpp"

namespace higanbana
{
  class ShaderArgumentsLayout
  {
    std::shared_ptr<ResourceHandle> m_id;
    std::shared_ptr<vector<std::string>> m_structs;
    std::shared_ptr<vector<ShaderResource>> m_resources;
  public:
    ShaderArgumentsLayout()
      : m_id(std::make_shared<ResourceHandle>())
      , m_structs(std::make_shared<vector<std::string>>())
      , m_resources(std::make_shared<vector<ShaderResource>>())
    {
    }

    ShaderArgumentsLayout(const ShaderArgumentsLayout&) = default;
    ShaderArgumentsLayout(ShaderArgumentsLayout&&) = default;
    ShaderArgumentsLayout& operator=(const ShaderArgumentsLayout&) = default;
    ShaderArgumentsLayout& operator=(ShaderArgumentsLayout&&) = default;

    ~ShaderArgumentsLayout()
    {
    }

    ShaderArgumentsLayout(std::shared_ptr<ResourceHandle> id, vector<std::string> structs, vector<ShaderResource> resources)
      : m_id(id)
      , m_structs(std::make_shared<vector<std::string>>(structs))
      , m_resources(std::make_shared<vector<ShaderResource>>(resources))
    {
    }

    vector<std::string>& structs()
    {
      return *m_structs;
    }
    vector<ShaderResource>& resources()
    {
      return *m_resources;
    }

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };
  
  class ShaderArguments
  {
    std::shared_ptr<ResourceHandle> m_id;
  public:
    ShaderArguments()
      : m_id(std::make_shared<ResourceHandle>())
    {
    }

    ShaderArguments(const ShaderArguments&) = default;
    ShaderArguments(ShaderArguments&&) = default;
    ShaderArguments& operator=(const ShaderArguments&) = default;
    ShaderArguments& operator=(ShaderArguments&&) = default;

    ~ShaderArguments()
    {
    }

    ShaderArguments(std::shared_ptr<ResourceHandle> id)
      : m_id(id)
    {
    }

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };
};