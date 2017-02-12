#pragma once

#include "backend.hpp"

#include "heap_descriptor.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/system/PageAllocator.hpp"
#include "core/src/datastructures/proxy.hpp"
#include <string>

namespace faze
{
  enum class GraphicsApi
  {
    Vulkan,
    DX12
  };

  enum class DeviceType
  {
    Unknown,
    IntegratedGpu,
    DiscreteGpu,
    VirtualGpu,
    Cpu
  };

  struct GpuInfo
  {
    int id;
    std::string name;
    int64_t memory;
    int vendor;
    DeviceType type;
    bool canPresent;
  };

  struct MemoryRequirements
  {
    size_t alignment;
    size_t bytes;
  };

  struct GpuHeapAllocation
  {
    int alignment;
    int index;
    PageBlock block;

    bool valid() { return alignment != -1 && index != -1; }
  };

  namespace backend
  {
    namespace prototypes
    {
      class DeviceImpl;
      class SubsystemImpl;
      class HeapImpl;
    }

    struct GpuHeap
    {
      std::shared_ptr<prototypes::HeapImpl> impl;
      HeapDescriptor desc;

      GpuHeap(std::shared_ptr<prototypes::HeapImpl> impl, HeapDescriptor desc)
        : impl(impl)
        , desc(std::move(desc))
      {
      }
    };

    class HeapManager
    {
      struct HeapBlock
      {
        int index;
        PageAllocator allocator;
        GpuHeap heap;
      };

      struct HeapVector
      {
        int alignment;
        vector<HeapBlock> heaps;
      };

      vector<HeapVector> m_heaps;
    public:
      GpuHeapAllocation allocate(MemoryRequirements requirements);
      void release(GpuHeapAllocation allocation);
    };

    struct DeviceData : std::enable_shared_from_this<DeviceData>
    {
      std::shared_ptr<prototypes::DeviceImpl> impl;
      HeapManager heaps;

      DeviceData(std::shared_ptr<prototypes::DeviceImpl> impl)
        : impl(impl)
      {
      }
    };

    struct SubsystemData : std::enable_shared_from_this<SubsystemData>
    {
      std::shared_ptr<prototypes::SubsystemImpl> impl;
      const char* appName;
      unsigned appVersion;
      const char* engineName;
      unsigned engineVersion;

      SubsystemData(GraphicsApi api, const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
      GpuDevice createDevice(FileSystem& fs, GpuInfo gpu);
    };
  }
}