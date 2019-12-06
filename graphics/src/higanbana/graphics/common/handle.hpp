#pragma once

#include "higanbana/graphics/desc/formats.hpp"
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/global_debug.hpp>
#include <higanbana/core/system/memview.hpp>
#include <stdint.h>
#include <type_traits>

namespace higanbana
{
  // any types that might be used in commandbuffers
  // commandbuffer doesn't track resources
  enum class ResourceType : uint64_t
  {
    Unknown,
    Pipeline,
    Renderpass,
    Buffer,
    DynamicBuffer,
    ReadbackBuffer,
    Texture,
    ReadbackTexture,
    MemoryHeap,
    ShaderArgumentsLayout,
    ShaderArguments,
    // Insert some raytracing things here?
    Count
  };

  enum class ViewResourceType : uint64_t
  {
    Unknown,
    BufferSRV,
    BufferUAV,
    BufferIBV,
    DynamicBufferSRV,
    TextureSRV,
    TextureUAV,
    TextureRTV,
    TextureDSV,
    Count
  };

  // lets just use 64bits for this
  // we can reduce the size later if needed
  struct ResourceHandle
  {
    static const uint64_t InvalidId = (1ull << 20ull) - 1;
    static const uint64_t AllGpus = 65535;
    union
    {
      struct 
      {
        uint64_t id : 20; // million is too much for id's
        uint64_t generation : 8; // generous generation id
        ResourceType type : 6; // ... I don't really want to write this much api types
        uint64_t gpuid : 16; // this should just be a bitfield, one bit for gpu, starting with modest 16 gpu's =D
        uint64_t m_usage : 4;
        uint64_t unused : 10; // honestly could be more bits here, lets just see how things go on 
      };
      uint64_t rawValue;
    };
    ResourceHandle()
      : id(InvalidId)
      , generation(0)
      , type(ResourceType::Unknown)
      , gpuid(0)
      {}
    ResourceHandle(uint64_t id, uint64_t generation, ResourceType type, uint64_t gpuID)
      : id(id)
      , generation(generation)
      , type(type)
      , gpuid(gpuID)
    {
      static_assert(std::is_standard_layout<ResourceHandle>::value,  "ResourceHandle should be trivial to destroy.");
      static_assert(sizeof(ResourceHandle) == 8,  "ViewRResourceHandle should be 128bits");
    }

    // returns positive value when single gpu
    // -1 when every gpu owns its own
    // -2 is mysterious error situation.
    int ownerGpuId() const
    {
      uint64_t c_gpuid = gpuid;
      uint64_t count = 0;
      if (c_gpuid == 65535)
      {
        return -1; 
      }
      while(count < 16) // valid id's 0-15
      {
        if (c_gpuid & (1 << count))
        {
          return int(count);
        }
        count++;
      }
      return -2;
    }

    void setGpuId(int id)
    {
      gpuid = (1 << id);
    }

    // shared resource means it's a handle that can be opened on another api/gpu device
    // otherwise all gpu's have their own version of this resource.
    bool sharedResource() const
    {
      return ownerGpuId() >= 0; // false for now
    }

    void setUsage(ResourceUsage usage)
    {
      m_usage = static_cast<uint64_t>(usage);
    }

    ResourceUsage usage() const
    {
      return static_cast<ResourceUsage>(m_usage);
    }

    bool operator==(const ResourceHandle& other) const
    {
      return id == other.id;
    }
    bool operator!=(const ResourceHandle& other) const
    {
      return id != other.id;
    }
  };

  // Saving some space by making specific type for views
  struct ViewResourceHandle
  {
    static const uint64_t InvalidViewId = (1ull << 14ull) - 1;
    union
    {
      /*
        14 + 8 + 4 + 4 + 4 + 4 + 11 + 11 + 2 + 1 + 1 = 64
       */
      struct 
      {
        uint64_t id : 14;
        uint64_t generation : 8;
        ViewResourceType type : 4;
        uint64_t m_allMips : 4;
        uint64_t m_startMip : 4;
        uint64_t m_mipSize : 4;
        uint64_t m_startArr : 11;
        uint64_t m_arrSize : 11;
        uint64_t m_loadop : 2;
        uint64_t m_storeop : 1;
        uint64_t unused : 1;
        ResourceHandle resource;
      };
      struct 
      {
        uint64_t rawView;
        uint64_t rawResource;
      };
    };
    ViewResourceHandle()
      : id(InvalidViewId)
      , generation(0)
      , type(ViewResourceType::Unknown)
      , resource(ResourceHandle(ResourceHandle::InvalidId, 0, ResourceType::Unknown, 0))
      {}
    ViewResourceHandle(uint64_t id, uint64_t generation, ViewResourceType type)
      : id(id)
      , generation(generation)
      , type(type)
      , resource(ResourceHandle(ResourceHandle::InvalidId, 0, ResourceType::Unknown, 0))
    {
      static_assert(std::is_standard_layout<ViewResourceHandle>::value,  "ResourceHandle should be trivial to destroy.");
      static_assert(sizeof(ViewResourceHandle) == 16,  "ViewRResourceHandle should be 128bits");
    }

    void subresourceRange(int fullMipSize, int startMip, int mipSize, int startArr, int arrSize)
    {
      m_allMips = static_cast<uint64_t>(fullMipSize - 1);
      m_startMip = static_cast<uint64_t>(startMip);
      m_mipSize = static_cast<uint64_t>(mipSize - 1);
      m_startArr = static_cast<uint64_t>(startArr);
      m_arrSize = static_cast<uint64_t>(arrSize - 1);
    }

    unsigned fullMipSize() const
    {
      return static_cast<unsigned>(m_allMips+1);
    }
    unsigned startMip() const
    {
      return static_cast<unsigned>(m_startMip);
    }
    unsigned mipSize() const
    {
      return static_cast<unsigned>(m_mipSize+1);
    }
    unsigned startArr() const
    {
      return static_cast<unsigned>(m_startArr);
    }
    unsigned arrSize() const
    {
      return static_cast<unsigned>(m_arrSize+1);
    }

    void setLoadOp(LoadOp op)
    {
      m_loadop = static_cast<uint64_t>(op);
    }
    void setStoreOp(StoreOp op)
    {
      m_storeop = static_cast<uint64_t>(op);
    }

    LoadOp loadOp() const
    {
      return static_cast<LoadOp>(m_loadop);
    }
    StoreOp storeOp() const
    {
      return static_cast<StoreOp>(m_storeop);
    }
  };

  /*
  Problem is how we allocate these and ...
  There probably should be a manager that we can use for checks
  
  Ideally we want to check generation on each access of resource.
  So we could have something that gives out id's and offers functions to check if those id's are expired.
  So we would need
    - Allocate Handle
      - get id from pool
        - if no id in pool, make a new one
    - Delete Handle
      - mostly just, return to pool as usable
    - check if alive
      - array to check current status of the id and match generation for id check
  */

  // we need legopiece to generate id's which knows how to reuse them
  // we need "type" amount of these lego pieces, all ranges begin from 0 till something

  // pool that grows depending how many maximum units are taken to a limit size.
  class HandlePool
  {
    vector<uint32_t> m_freelist;
    vector<uint8_t> m_generation;
    ResourceType m_type = ResourceType::Unknown;
    uint64_t m_currentSize = 0;
    uint64_t m_size = 0;
  public:
    HandlePool(ResourceType type, int size)
      : m_type(type)
      , m_size(static_cast<uint64_t>(size))
    {
    }

    ResourceHandle allocate()
    {
      if (m_freelist.empty() && m_currentSize+1 < m_size)
      {
        m_generation.push_back(0);
        auto id = m_currentSize;
        m_currentSize++;
        return ResourceHandle{id, 0, m_type, ResourceHandle::AllGpus};
      }
      HIGAN_ASSERT(!m_freelist.empty(), "No free handles.");
      auto id = m_freelist.back();
      m_freelist.pop_back();
      auto generation = m_generation[id]; // take current generation
      return ResourceHandle{id, generation, m_type, ResourceHandle::AllGpus};
    }

    void release(ResourceHandle val)
    {
      HIGAN_ASSERT(val.id != ResourceHandle::InvalidId, "Invalid handle was released.");
      HIGAN_ASSERT(val.id < m_size, "Invalid handle was released.");
      HIGAN_ASSERT(val.generation == m_generation[val.id], "Invalid handle was released.");
      m_freelist.push_back(val.id);
      m_generation[val.id]++; // offset the generation to detect double free's
    }

    bool valid(ResourceHandle handle)
    {
      return handle.id != ResourceHandle::InvalidId && handle.id < m_size && handle.generation == m_generation[handle.id];
    }

    size_t size() const
    {
      return m_freelist.size();
    }
  };

  class ViewHandlePool
  {
    vector<uint32_t> m_freelist;
    vector<uint8_t> m_generation;
    ViewResourceType m_type = ViewResourceType::Unknown;
    uint64_t m_currentSize = 0;
    uint64_t m_size = 0;
  public:
    ViewHandlePool(ViewResourceType type, int size)
      : m_type(type)
      , m_size(static_cast<uint64_t>(size))
    {
    }

    ViewResourceHandle allocate()
    {
      if (m_freelist.empty() && m_currentSize+1 < m_size)
      {
        m_generation.push_back(0);
        auto id = m_currentSize;
        m_currentSize++;
        return ViewResourceHandle(id, 0, m_type);
      }
      HIGAN_ASSERT(!m_freelist.empty(), "No free handles.");
      auto id = m_freelist.back();
      m_freelist.pop_back();
      auto generation = m_generation[id]; // take current generation
      return ViewResourceHandle(id, generation, m_type);
    }

    void release(ViewResourceHandle val)
    {
      HIGAN_ASSERT(val.id != ViewResourceHandle::InvalidViewId, "Invalid view handle was released.");
      HIGAN_ASSERT(val.id < m_size, "Invalidview handle was released.");
      HIGAN_ASSERT(val.generation == m_generation[val.id], "Invalid view handle was released.");
      m_freelist.push_back(val.id);
      m_generation[val.id]++; // offset the generation to detect double free's
    }

    bool valid(ViewResourceHandle handle)
    {
      return handle.id != ViewResourceHandle::InvalidViewId && handle.id < m_size && handle.generation == m_generation[handle.id];
    }

    size_t size() const
    {
      return m_freelist.size();
    }
  };

  class HandleManager
  {
    vector<HandlePool> m_pools;
    vector<ViewHandlePool> m_views;
  public:
    HandleManager(int poolSizes = 1024*64)
    {
      for (int i = 0; i < static_cast<int>(ResourceType::Count); ++i)
      {
        ResourceType type = static_cast<ResourceType>(i);
        if (type == ResourceType::Unknown)
          continue;
        m_pools.push_back(HandlePool(type, poolSizes));
      }
      for (int i = 0; i < static_cast<int>(ViewResourceType::Count); ++i)
      {
        ViewResourceType type = static_cast<ViewResourceType>(i);
        if (type == ViewResourceType::Unknown)
          continue;
        m_views.push_back(ViewHandlePool(type, poolSizes));
      }
    }

    ResourceHandle allocateResource(ResourceType type)
    {
      HIGAN_ASSERT(type != ResourceType::Unknown, "please valide type.");
      int index = static_cast<int>(type) - 1;
      auto& pool = m_pools[index];
      return pool.allocate();
    }

    ViewResourceHandle allocateViewResource(ViewResourceType type, ResourceHandle resource)
    {
      HIGAN_ASSERT(type != ViewResourceType::Unknown, "please valide type.");
      int index = static_cast<int>(type) - 1;
      auto& pool = m_views[index];
      auto view = pool.allocate();
      view.resource = resource;
      return view;
    }

    void release(ResourceHandle handle)
    {
      HIGAN_ASSERT(handle.type != ResourceType::Unknown, "please valied type");
      int typeIndex = static_cast<int>(handle.type) - 1;
      auto& pool = m_pools[typeIndex];
      pool.release(handle);
    }

    bool valid(ResourceHandle handle)
    {
      if (handle.type == ResourceType::Unknown)
      {
        return false;
      }
      int typeIndex = static_cast<int>(handle.type);
      auto& pool = m_pools[typeIndex];
      return pool.valid(handle);
    }
    void release(ViewResourceHandle handle)
    {
      HIGAN_ASSERT(handle.type != ViewResourceType::Unknown, "please valied type");
      int typeIndex = static_cast<int>(handle.type) - 1;
      auto& pool = m_views[typeIndex];
      pool.release(handle);
    }

    bool valid(ViewResourceHandle handle)
    {
      if (handle.type == ViewResourceType::Unknown)
      {
        return false;
      }
      int typeIndex = static_cast<int>(handle.type);
      auto& pool = m_views[typeIndex];
      return pool.valid(handle);
    }
  };

  template <typename Type>
  class HandleVector
  {
    vector<Type> objects;
    public:
    HandleVector(){}

    Type& operator[](ResourceHandle handle)
    {
      if (objects.size() < handle.id+1)
      {
        objects.resize(handle.id+1);
      }
      return objects[handle.id]; 
    }

    Type& operator[](ViewResourceHandle handle)
    {
      if (objects.size() < handle.id+1)
      {
        objects.resize(handle.id+1);
      }
      return objects[handle.id]; 
    }

    Type& at(int id)
    {
      return objects.at(id); 
    }

    MemView<Type> view()
    {
      return makeMemView(objects);
    }

    void clear()
    {
      objects.clear();
    }
  };
}