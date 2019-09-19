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
    std::shared_ptr<DynamicBitfield> m_referencedBuffers;
    std::shared_ptr<DynamicBitfield> m_referencedTextures;

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
            m_referencedBuffers->setBit(view.resource.id);
            break;
          }
          case ViewResourceType::TextureSRV:
          case ViewResourceType::TextureUAV:
          case ViewResourceType::TextureRTV:
          case ViewResourceType::TextureDSV:
          {
            m_referencedTextures->setBit(view.resource.id);
            break;
          }
          default:
            break;
        }
      }
    }
  public:
    ShaderArguments()
      : m_id(std::make_shared<ResourceHandle>())
      , m_referencedBuffers(std::make_shared<DynamicBitfield>())
      , m_referencedTextures(std::make_shared<DynamicBitfield>())
    {
    }

    ShaderArguments(const ShaderArguments&) = default;
    ShaderArguments(ShaderArguments&&) = default;
    ShaderArguments& operator=(const ShaderArguments&) = default;
    ShaderArguments& operator=(ShaderArguments&&) = default;

    ~ShaderArguments()
    {
    }

    ShaderArguments(std::shared_ptr<ResourceHandle> id, MemView<ViewResourceHandle> views)
      : m_id(id)
      , m_referencedBuffers(std::make_shared<DynamicBitfield>())
      , m_referencedTextures(std::make_shared<DynamicBitfield>())
    {
      addMemview(views);
    }

    const DynamicBitfield& refBuffers() const
    {
      return *m_referencedBuffers;
    }

    const DynamicBitfield& refTextures() const
    {
      return *m_referencedTextures;
    }

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };
};