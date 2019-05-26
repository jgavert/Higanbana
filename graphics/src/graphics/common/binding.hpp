#pragma once

#include "graphics/common/buffer.hpp"
#include "graphics/common/texture.hpp"
#include "graphics/common/pipeline.hpp"

#include <core/datastructures/proxy.hpp>


namespace faze
{
  class Binding
  {
    friend class CommandGraphNode;
    vector<ShaderResource> m_resources;
    vector<backend::RawView> m_views;
    vector<backend::TrackedState> m_res;
    vector<uint8_t> m_constants;

    Binding(GraphicsPipeline pipeline)
    : m_resources(pipeline.descriptor.layout.desc.sortedResources)
    , m_views(m_resources.size())
    , m_res(m_resources.size())
    , m_constants(pipeline.descriptor.layout.desc.constantsSizeOf)
    {

    }
    Binding(ComputePipeline pipeline)
    : m_resources(pipeline.descriptor.layout.desc.sortedResources)
    , m_views(m_resources.size())
    , m_res(m_resources.size())
    , m_constants(pipeline.descriptor.layout.desc.constantsSizeOf)
    {
      
    }

    MemView<backend::TrackedState> bResources()
    {
      return MemView<backend::TrackedState>(m_res);
    }

    MemView<backend::RawView> bViews()
    {
      return MemView<backend::RawView>(m_views);
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
          F_ASSERT(it.readonly, "Trying to bind DynamicBufferView \"%s\" as ReadWrite.", name);
          m_views[id] = res.view();
          m_res[id] = backend::TrackedState{ 0,0,0, true };
          return;
        }
        id++;
      }
      F_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const BufferSRV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          F_ASSERT(it.readonly, "Trying to bind BufferSRV \"%s\" as ReadWrite.", name);
          m_views[id] = res.view();
          m_res[id] = res.dependency();
          m_res[id].readonly = true;
          return;
        }
        id++;
      }
      F_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const TextureSRV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name))
        {
          F_ASSERT(it.readonly, "Trying to bind TextureSRV \"%s\" as ReadWrite.", name);
          m_views[id] = res.view();
          m_res[id] = res.dependency();
          m_res[id].readonly = true;
          return;
        }
        id++;
      }
      F_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const BufferUAV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          F_ASSERT(!it.readonly, "Trying to bind BufferUAV \"%s\" as ReadOnly.", name);
          m_views[id] = res.view();
          m_res[id] = res.dependency();
          m_res[id].readonly = false;
          return;
        }
        id++;
      }
      F_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
    void bind(const char* name, const TextureUAV& res)
    {
      int id = 0;
      for (auto&& it : m_resources)
      {
        if (it.name.compare(name) == 0)
        {
          F_ASSERT(!it.readonly, "Trying to bind TextureUAV \"%s\" as ReadOnly.", name);
          m_views[id] = res.view();
          m_res[id] = res.dependency();
          m_res[id].readonly = false;
          return;
        }
        id++;
      }
      F_ASSERT(false, "No such resource declared as \"%s\". Look at shaderinputs.", name);
    }
  };
}