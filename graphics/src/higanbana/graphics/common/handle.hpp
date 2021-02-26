#pragma once

#include "higanbana/graphics/desc/formats.hpp"
#include <higanbana/core/datastructures/vector.hpp>
#include <higanbana/core/system/memview.hpp>
#include <stdint.h>
#include <type_traits>
#include <mutex>

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
    RaytracingAccelerationStructure,
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
        uint64_t sharedResource : 1;
        uint64_t m_allMips : 4; // needed so often, just store it here
        uint64_t unused : 5; // honestly could be more bits here, lets just see how things go on 
      };
      uint64_t rawValue;
    };
    ResourceHandle()
      : id(InvalidId)
      , generation(0)
      , type(ResourceType::Unknown)
      , gpuid(0)
      , sharedResource(0)
      , m_allMips(0)
      , unused(0)
      {}
    ResourceHandle(uint64_t id, uint64_t generation, ResourceType type, uint64_t gpuID, bool isShared)
      : id(id)
      , generation(generation)
      , type(type)
      , gpuid(gpuID)
      , sharedResource(isShared ? 1 : 0)
      , m_allMips(0)
      , unused(0)
    {
      static_assert(std::is_standard_layout<ResourceHandle>::value,  "ResourceHandle should be trivial to destroy.");
      static_assert(sizeof(ResourceHandle) == 8,  "ResourceHandle should be 64bits");
    }

    void setMipCount(uint fullMipSize) {
      m_allMips = static_cast<uint64_t>(fullMipSize - 1);
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
    bool shared() const
    {
      return sharedResource;
    }

    void setUsage(ResourceUsage usage)
    {
      m_usage = static_cast<uint64_t>(usage);
    }

    unsigned fullMipSize() const
    {
      return static_cast<unsigned>(m_allMips+1);
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
      return !operator==(other);
    }
    explicit operator bool() const {
      return id != InvalidId;
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
        uint64_t m_startMip : 4;
        uint64_t m_mipSize : 4;
        uint64_t m_startArr : 11;
        uint64_t m_arrSize : 11;
        uint64_t m_loadop : 2;
        uint64_t m_storeop : 1;
        uint64_t unused : 5;
        uint64_t resource : 64;
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
      , resource(ResourceHandle(ResourceHandle::InvalidId, 0, ResourceType::Unknown, 0, false).rawValue)
      {}
    ViewResourceHandle(uint64_t id, uint64_t generation, ViewResourceType type)
      : id(id)
      , generation(generation)
      , type(type)
      , resource(ResourceHandle(ResourceHandle::InvalidId, 0, ResourceType::Unknown, 0, false).rawValue)
    {
      static_assert(std::is_standard_layout<ViewResourceHandle>::value,  "ResourceHandle should be trivial to destroy.");
      static_assert(sizeof(ViewResourceHandle) == 16,  "ViewRResourceHandle should be 128bits");
    }

    ResourceHandle resourceHandle() const { ResourceHandle h; h.rawValue = resource; return h;}

    void subresourceRange(int startMip, int mipSize, int startArr, int arrSize)
    {
      m_startMip = static_cast<uint64_t>(startMip);
      m_mipSize = static_cast<uint64_t>(mipSize - 1);
      m_startArr = static_cast<uint64_t>(startArr);
      m_arrSize = static_cast<uint64_t>(arrSize - 1);
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

    constexpr LoadOp loadOp() const
    {
      return static_cast<LoadOp>(m_loadop);
    }
    constexpr StoreOp storeOp() const
    {
      return static_cast<StoreOp>(m_storeop);
    }

    constexpr bool operator==(const ViewResourceHandle& other) const 
    {
      return rawView == other.rawView && rawResource == other.rawResource;
    }
    constexpr bool operator!=(const ViewResourceHandle& other) const
    {
      return !operator==(other);
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
    HandlePool(ResourceType type, int size);
    ResourceHandle allocate();
    void release(ResourceHandle val);
    bool valid(ResourceHandle handle);
    size_t size() const;
  };

  class ViewHandlePool
  {
    vector<uint32_t> m_freelist;
    vector<uint8_t> m_generation;
    ViewResourceType m_type = ViewResourceType::Unknown;
    uint64_t m_currentSize = 0;
    uint64_t m_size = 0;
  public:
    ViewHandlePool(ViewResourceType type, int size);
    ViewResourceHandle allocate();
    void release(ViewResourceHandle val);
    bool valid(ViewResourceHandle handle);
    size_t size() const;
  };

  class HandleManager
  {
    vector<HandlePool> m_pools;
    vector<ViewHandlePool> m_views;
    std::shared_ptr<std::mutex> m_handleMutex;
  public:
    HandleManager(int poolSizes = 1024*64);
    ResourceHandle allocateResource(ResourceType type);
    ViewResourceHandle allocateViewResource(ViewResourceType type, ResourceHandle resource);
    void release(ResourceHandle handle);
    bool valid(ResourceHandle handle);
    void release(ViewResourceHandle handle);
    bool valid(ViewResourceHandle handle);
  };

  template <typename Type>
  class HandleVector
  {
    vector<Type> objects;
    public:
    HandleVector(){
      objects.resize(1024);
    }

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
      return MemView(objects);
    }

    void clear()
    {
      objects.clear();
    }
  };
}