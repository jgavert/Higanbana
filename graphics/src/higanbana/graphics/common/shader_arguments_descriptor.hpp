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
    vector<ViewResourceHandle> m_handles;

  public:
    ShaderArgumentsDescriptor(std::string name, ShaderArgumentsLayout layout)
    : m_name(name)
    , m_layout(layout.handle())
    , m_resources(layout.resources())
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

    const vector<ViewResourceHandle>& bResources() const
    {
      return m_handles;
    }

    void bind(const char* name, const DynamicBufferView& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(it.readonly, "Trying to bind DynamicBufferView \"%s\" as ReadWrite.", name);
          m_handles[id] = res.handle();
          return;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const BufferSRV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(it.readonly, "Trying to bind BufferSRV \"%s\" as ReadWrite.", name);
          m_handles[id] = res.handle();
          return;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const TextureSRV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(it.readonly, "Trying to bind TextureSRV \"%s\" as ReadWrite.", name);
          m_handles[id] = res.handle();
          return;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const BufferUAV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(!it.readonly, "Trying to bind BufferUAV \"%s\" as ReadOnly.", name);
          m_handles[id] = res.handle();
          return;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const TextureUAV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          HIGAN_ASSERT(!it.readonly, "Trying to bind TextureUAV \"%s\" as ReadOnly.", name);
          m_handles[id] = res.handle();
          return;
        }
        id++;
      }
      HIGAN_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
  };
}