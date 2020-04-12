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
    std::shared_ptr<vector<std::pair<size_t, std::string>>> m_structs;
    std::shared_ptr<vector<ShaderResource>> m_resources;
    std::shared_ptr<ShaderResource> m_bindless;
  public:
    ShaderArgumentsLayout()
      : m_id(std::make_shared<ResourceHandle>())
      , m_structs(std::make_shared<vector<std::pair<size_t, std::string>>>())
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

    ShaderArgumentsLayout(std::shared_ptr<ResourceHandle> id, vector<std::pair<size_t, std::string>> structs, vector<ShaderResource> resources, ShaderResource bindless)
      : m_id(id)
      , m_structs(std::make_shared<vector<std::pair<size_t, std::string>>>(structs))
      , m_resources(std::make_shared<vector<ShaderResource>>(resources))
      , m_bindless(std::make_shared<ShaderResource>(bindless)) {
    }

    vector<std::pair<size_t, std::string>>& structs() {
      return *m_structs;
    }
    vector<ShaderResource>& resources() {
      return *m_resources;
    }
    ShaderResource bindless() {
      return *m_bindless;
    }

    ResourceHandle handle() const {
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

    void addMemview(const vector<ViewResourceHandle>& m_resources)
    {
      for (auto&& view : m_resources)
      {
        switch (view.type)
        {
          case ViewResourceType::BufferSRV:
          case ViewResourceType::BufferUAV:
          case ViewResourceType::BufferIBV:
          {
            m_referencedBuffers->setBit(view.resourceHandle().id);
            break;
          }
          case ViewResourceType::TextureSRV:
          case ViewResourceType::TextureUAV:
          case ViewResourceType::TextureRTV:
          case ViewResourceType::TextureDSV:
          {
            m_referencedTextures->setBit(view.resourceHandle().id);
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

    ShaderArguments(std::shared_ptr<ResourceHandle> id, const vector<ViewResourceHandle>& views)
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

    explicit operator bool() const
    {
      return handle().id != ResourceHandle::InvalidId && m_referencedBuffers && m_referencedTextures;
    }
  };
};