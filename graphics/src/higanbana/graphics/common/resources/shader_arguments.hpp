#pragma once

#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/handle.hpp"

namespace higanbana
{
  class ShaderArgumentsLayout
  {
    std::shared_ptr<ResourceHandle> m_id;
    //std::shared_ptr<ResourceDescriptor> m_desc;
  public:
    ShaderArgumentsLayout()
      : m_id(std::make_shared<ResourceHandle>())
      //, m_desc(std::make_shared<ResourceDescriptor>())
    {
    }

    ShaderArgumentsLayout(const ShaderArgumentsLayout&) = default;
    ShaderArgumentsLayout(ShaderArgumentsLayout&&) = default;
    ShaderArgumentsLayout& operator=(const ShaderArgumentsLayout&) = default;
    ShaderArgumentsLayout& operator=(ShaderArgumentsLayout&&) = default;

    ~ShaderArgumentsLayout()
    {
    }

    ShaderArgumentsLayout(std::shared_ptr<ResourceHandle> id)
      : m_id(id)
      //, m_desc(desc)
    {
    }

    /*
    ResourceDescriptor& desc()
    {
      return *m_desc;
    }
    */

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
    //std::shared_ptr<ResourceDescriptor> m_desc;
  public:
    ShaderArguments()
      : m_id(std::make_shared<ResourceHandle>())
      //, m_desc(std::make_shared<ResourceDescriptor>())
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
      //, m_desc(desc)
    {
    }

    /*
    ResourceDescriptor& desc()
    {
      return *m_desc;
    }
    */

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };
};