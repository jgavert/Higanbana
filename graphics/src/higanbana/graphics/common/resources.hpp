#pragma once
#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/handle.hpp"

#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/entity/bitfield.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <higanbana/core/system/heap_allocator.hpp>
#include <memory>


namespace higanbana
{
  enum class QueryDevicesMode
  {
    UseCached,
    SlowQuery
  };

  enum class GraphicsApi
  {
	  All,
    Vulkan,
    DX12,
  };

  const char* toString(GraphicsApi api);

  enum class DeviceType
  {
    Unknown,
    IntegratedGpu,
    DiscreteGpu,
    VirtualGpu,
    Cpu
  };

  enum class VendorID
  {
    Unknown,
    All,
    Amd, // = 4098,
    Nvidia, // = 4318,
    Intel // = 32902
  };
  const char* toString(VendorID vendor);

  enum QueueType : uint32_t
  {
    Unknown = 0,
    Dma = 0x1,
    Compute = 0x2 | QueueType::Dma,
    Graphics = 0x4 | QueueType::Compute,
    External = 0x8
  };
  const char* toString(QueueType queue);

  struct GpuInfo
  {
    int id = -1;
    std::string name = "";
    int64_t memory = -1;
    VendorID vendor = VendorID::Unknown;
    uint deviceId = 0; // used to match 2 devices together for interopt
    DeviceType type = DeviceType::Unknown;
    bool gpuConstants = false;
    bool canPresent = false;
    bool canRaytrace = false;
    GraphicsApi api = GraphicsApi::All;
    uint32_t apiVersion = 0;
    std::string apiVersionStr = "";
  };

  enum class ThreadedSubmission
  {
    Sequenced,
    Parallel,
    ParallelUnsequenced,
    Unsequenced
  };

  struct MemoryRequirements
  {
    size_t alignment;
    size_t bytes;
    int64_t heapType;
  };

  struct GpuHeapAllocation;
  class FixedSizeAllocator;

  class SwapchainDescriptor;
  class ResourceDescriptor;
  class ShaderViewDescriptor;
  class ComputePipelineDescriptor;
  class ComputePipeline;
  class GraphicsPipelineDescriptor;
  class GraphicsPipeline;

  struct ResourceHandle;

  class Buffer;
  class SharedBuffer;
  class Texture;
  class SharedTexture;

  class BufferSRV;
  class BufferUAV;
  class TextureSRV;
  class TextureUAV;
  class TextureRTV;
  class TextureDSV;
  class DynamicBufferView;
  class ReadbackData;

  class ShaderArgumentsLayoutDescriptor;
  class ShaderArgumentsLayout;
  class ShaderArguments;

  class Swapchain;
  class Renderpass;
  class GraphicsSurface;
  class Window;
  class CommandGraph;
  class CpuImage;

  class GpuSemaphore;
  class GpuSharedSemaphore;

  class HeapDescriptor;

  namespace backend
  {
    class FenceImpl;
    class SemaphoreImpl;
    class CommandBufferImpl;
    class IntermediateList;
    class CommandBuffer;
    namespace prototypes
    {
      class DeviceImpl;
      class SubsystemImpl;
      class HeapImpl;
      class BufferImpl;
      class TextureImpl;
      class BufferViewImpl;
      class TextureViewImpl;
      class SwapchainImpl;
      class GraphicsSurfaceImpl;
    }

#if defined(HIGANBANA_PLATFORM_WINDOWS)
    struct SharedHandle
    {
      GraphicsApi api;
      void* handle;
      size_t heapsize;
    };
#else
    struct SharedHandle
    {
      int woot;
    };
#endif

    struct GpuHeap
    {
      ResourceHandle handle;
      std::shared_ptr<HeapDescriptor> desc;

      GpuHeap(ResourceHandle handle, HeapDescriptor desc);
    };

    struct HeapAllocation
    {
      GpuHeapAllocation allocation;
      GpuHeap heap;
    };

    class HeapManager
    {
      struct HeapBlock
      {
        uint64_t index;
        HeapAllocator allocator;
        GpuHeap heap;
      };

      struct HeapVector
      {
        int64_t type;
        vector<HeapBlock> heaps;
      };

      vector<HeapVector> m_heaps;
      const int64_t m_minimumHeapSize = 16 * 1024 * 1024; // todo: move this configuration elsewhere
      uint64_t m_heapIndex = 0;

      uint64_t m_memoryAllocated = 0;
      uint64_t m_totalMemory = 0;
    public:

      HeapAllocation allocate(MemoryRequirements requirements, std::function<GpuHeap(HeapDescriptor)> allocator);
      void release(GpuHeapAllocation allocation);
      vector<GpuHeap> emptyHeaps();
      uint64_t memoryInUse();
      uint64_t totalMemory();
    };

    struct QueueStates
    {
      DynamicBitfield gb;
      DynamicBitfield gt;
      DynamicBitfield cb;
      DynamicBitfield ct;
      DynamicBitfield db;
      DynamicBitfield dt;
      // DynamicBitfield eb;
      // DynamicBitfield et;
    };
  }
}