#pragma once

#include "higanbana/graphics/common/buffer.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/pipeline.hpp"
#include "higanbana/graphics/common/handle.hpp"

#include <higanbana/core/datastructures/proxy.hpp>


namespace higanbana
{
  class Binding
  {
    friend class CommandGraphNode;
    vector<ShaderResource> m_resources;
    vector<ViewResourceHandle> m_handles;
    vector<uint8_t> m_constants;
    uint3 m_baseGroups;

    Binding(GraphicsPipeline pipeline)
    : m_resources(pipeline.descriptor.desc.layout.sortedResources)
    , m_handles(m_resources.size())
    , m_constants(pipeline.descriptor.desc.layout.constantsSizeOf)
    {

    }
    Binding(ComputePipeline pipeline)
    : m_resources(pipeline.descriptor.layout.sortedResources)
    , m_handles(m_resources.size())
    , m_constants(pipeline.descriptor.layout.constantsSizeOf)
    , m_baseGroups(pipeline.descriptor.shaderGroups)
    {
      
    }

    MemView<ViewResourceHandle> bResources()
    {
      return MemView<ViewResourceHandle>(m_handles);
    }

    MemView<uint8_t> bConstants()
    {
      return MemView<uint8_t>(m_constants);
    }

  public:
    template <typename T>
    void constants(T consts)
    {
      m_constants.resize(sizeof(T));
      memcpy(m_constants.data(), &consts, sizeof(T));
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

    uint3 baseGroups()
    {
      return m_baseGroups;
    }
  };
}