#pragma once

#include <core/datastructures/proxy.hpp>
#include <core/global_debug.hpp>

#include <stdint.h>
#include <type_traits>

namespace faze
{
  // any types that might be used in commandbuffers
  // commandbuffer doesn't track resources
  enum class ResourceType : uint64_t
  {
    Unknown,
    Pipeline,
    Buffer,
    DynamicBuffer,
    Texture,
    BufferSRV,
    BufferUAV,
    BufferIBV,
    TextureSRV,
    TextureUAV,
    TextureRTV,
    TextureDSV,
    // Insert some raytracing things here?
    Count
  };

  // lets just use 64bits for this
  // we can reduce the size later if needed
  struct ResourceHandle
  {
    static const uint64_t InvalidId = (1ull << 20ull) - 1;
    union
    {
      struct 
      {
        uint64_t id : 20; // million is too much for id's
        uint64_t generation : 8; // generous generation id
        ResourceType type : 6; // ... I don't really want to write this much api types
        uint64_t gpuid : 16; // this should just be a bitfield, one bit for gpu, starting with modest 16 gpu's =D
        uint64_t unused : 14; // honestly could be more bits here, lets just see how things go on 
      };
      uint64_t rawValue;
    };
    ResourceHandle(uint64_t id, uint64_t generation, ResourceType type, uint64_t gpuID)
      : id(id)
      , generation(generation)
      , type(type)
      , gpuid(gpuID)
    {
      static_assert(std::is_standard_layout<ResourceHandle>::value,  "ResourceHandle should be trivial to destroy.");
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
      }
      return -2;
    }

    // shared resource means it's a handle that can be opened on another api/gpu device
    // otherwise all gpu's have their own version of this resource.
    bool sharedResource() const
    {
      return ownerGpuId() >= 0; // false for now
    }
  };
  static const ResourceHandle InvalidResourceHandle = ResourceHandle(ResourceHandle::InvalidId, 0, ResourceType::Unknown, 0);

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
        return ResourceHandle{id, 0, m_type, 0};
      }
      F_ASSERT(!m_freelist.empty(), "No free handles.");
      auto id = m_freelist.back();
      m_freelist.pop_back();
      auto generation = m_generation[id]; // take current generation
      return ResourceHandle{id, generation, m_type, 0};
    }

    void release(ResourceHandle val)
    {
      F_ASSERT(val.id != ResourceHandle::InvalidId
                && val.id < m_size
                && val.generation == m_generation[val.id]
                , "Invalid handle was released.");
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

  class HandleManager
  {
    vector<HandlePool> m_pools;
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
    }

    ResourceHandle allocate(ResourceType type)
    {
      F_ASSERT(type != ResourceType::Unknown, "please valide type.");
      int index = static_cast<int>(type) - 1;
      auto& pool = m_pools[index];
      return pool.allocate();
    }

    void release(ResourceHandle handle)
    {
      F_ASSERT(handle.type != ResourceType::Unknown, "please valied type");
      int typeIndex = static_cast<int>(handle.type);
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
  };

  template <typename Type>
  class HandleVector
  {
    vector<Type> objects;
    public:
    HandleVector(){}

    Type& operator[](ResourceHandle handle)
    {
      if (objects.size() < handle.id)
      {
        objects.resize(handle.id+1);
      }
      return objects[handle.id]; 
    }
  };
}