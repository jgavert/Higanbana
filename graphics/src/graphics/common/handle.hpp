#pragma once

#include <core/datastructures/proxy.hpp>
#include <core/global_debug.hpp>

#include <stdint.h>
#include <type_traits>

namespace faze
{
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
        uint64_t type : 6; // ... I don't really want to write this much api types
        uint64_t gpuid : 16; // this should just be a bitfield, one bit for gpu, starting with modest 16 gpu's =D
        uint64_t unused : 14; // honestly could be more bits here, lets just see how things go on 
      };
      uint64_t rawValue;
    };
    ResourceHandle(uint64_t id, uint64_t generation, uint64_t type, uint64_t gpuID)
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
  static const ResourceHandle InvalidResourceHandle = ResourceHandle(ResourceHandle::InvalidId, 0, 0, 0);

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

  class HandlePool
  {
    vector<uint32_t> m_freelist;
    vector<uint8_t> m_generation;
    uint64_t m_type = 0;
    int m_size = 0;
  public:
    HandlePool(uint64_t type, int size)
      : m_type(type)
      , m_size(size)
    {
      for (int i = size; i >= 0; i--)
      {
        m_freelist.push_back(i);
      }
      m_generation.resize(m_size);
      for (int i = 0; i < m_size; ++i)
      {
        m_generation[i] = 0;
      }
    }

    ResourceHandle allocate()
    {
      if (m_freelist.empty())
      {
        F_ASSERT(false, "No handles left, what.");
        return ResourceHandle{ResourceHandle::InvalidId, 0, m_type, 0};
      }
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
      F_ASSERT(handle.id != ResourceHandle::InvalidId
                && handle.id < m_size
                , "Invalid handle was passed.");
      return handle.generation == m_generation[handle.id];
    }

    size_t size() const
    {
      return m_freelist.size();
    }
  };

}