#pragma once

#include <stdint.h>
#include <type_traits>

namespace faze
{
  // lets just use 64bits for this
  // we can reduce the size later if needed
  struct ResourceHandle
  {
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
}