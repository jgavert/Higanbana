#pragma once

#include "higanbana/graphics/common/buffer.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/pipeline.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"

#include <higanbana/core/datastructures/proxy.hpp>

namespace higanbana
{
  class ShaderArgumentsDescriptor 
  {
    std::string m_name;
    ResourceHandle m_layout;
    vector<ShaderResource> m_resources;
    ShaderResource m_bindless;
    vector<ViewResourceHandle> m_handles;
    vector<ViewResourceHandle> m_bindlessHandles;

  public:
    ShaderArgumentsDescriptor(std::string name, ShaderArgumentsLayout layout)
    : m_name(name)
    , m_layout(layout.handle())
    , m_resources(layout.resources())
    , m_bindless(layout.bindless())
    , m_handles(m_resources.size())
    {

    }

    std::string name()
    {
      return m_name;
    }

    ResourceHandle layout()
    {
      return m_layout;
    }

    const vector<ShaderResource>& bDescriptors() const
    {
      return m_resources;
    }

    const vector<ViewResourceHandle>& bResources() const
    {
      return m_handles;
    }
    const ShaderResource& bBindlessDesc() const
    {
      return m_bindless;
    }
    const vector<ViewResourceHandle>& bBindless() const
    {
      return m_bindlessHandles;
    }

    ShaderArgumentsDescriptor& bind(const char* name, const DynamicBufferView& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(it.readonly, "Trying to bind DynamicBufferView \"%s\" as ReadWrite.", name);
          m_handles[id] = res.handle();
          return *this;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
      return *this;
    }
    ShaderArgumentsDescriptor& bind(const char* name, const BufferSRV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(it.readonly, "Trying to bind BufferSRV \"%s\" as ReadWrite.", name);
          m_handles[id] = res.handle();
          return *this;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
      return *this;
    }
    ShaderArgumentsDescriptor& bind(const char* name, const TextureSRV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(it.readonly, "Trying to bind TextureSRV \"%s\" as ReadWrite.", name);
          m_handles[id] = res.handle();
          return *this;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
      return *this;
    }
    ShaderArgumentsDescriptor& bind(const char* name, const BufferUAV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(!it.readonly, "Trying to bind BufferUAV \"%s\" as ReadOnly.", name);
          m_handles[id] = res.handle();
          return *this;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
      return *this;
    }
    ShaderArgumentsDescriptor& bind(const char* name, const TextureUAV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(!it.readonly, "Trying to bind TextureUAV \"%s\" as ReadOnly.", name);
          m_handles[id] = res.handle();
          return *this;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
      return *this;
    }

    ShaderArgumentsDescriptor& bindBindless(const char* name, const vector<TextureSRV>& res)
    {
      if (m_bindless.name.compare(name) == 0)
      {
        HIGAN_ASSERT(m_bindless.readonly, "Trying to bind BufferSRV \"%s\" as ReadWrite.", name);
        m_bindlessHandles.clear();
        for (auto&& it : res) {
          if (m_bindlessHandles.size() >= m_bindless.bindlessCountWorstCase)
            break;
          m_bindlessHandles.push_back(it.handle());
        }
        return *this;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
      return *this;
    }
  };
}
