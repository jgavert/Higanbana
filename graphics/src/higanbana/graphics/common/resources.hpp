#pragma once
#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/handle.hpp"

#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/entity/bitfield.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <memory>


namespace higanbana
{
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
    Amd, // = 4098,
    Nvidia, // = 4318,
    Intel // = 32902
  };
  const char* toString(VendorID vendor);

  enum class QueueType : uint32_t
  {
    Unknown,
    Graphics,
    Compute,
    Dma,
    External
  };
  const char* toString(QueueType queue);

  struct GpuInfo
  {
    int id;
    std::string name;
    int64_t memory;
    VendorID vendor;
    DeviceType type;
    bool canPresent;
    bool canRaytrace;
    GraphicsApi api;
    uint32_t apiVersion;
    std::string apiVersionStr;
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
      HANDLE handle;
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
        FixedSizeAllocator allocator;
        GpuHeap heap;
      };

      struct HeapVector
      {
        int alignment;
        int64_t type;
        vector<HeapBlock> heaps;
      };

      vector<HeapVector> m_heaps;
      const int64_t m_minimumHeapSize = 16 * 1024 * 1024; // todo: move this configuration elsewhere
      uint64_t m_heapIndex = 0;

      uint64_t m_totalMemory = 0;
    public:

      HeapAllocation allocate(MemoryRequirements requirements, std::function<GpuHeap(HeapDescriptor)> allocator);
      void release(GpuHeapAllocation allocation);

      vector<GpuHeap> emptyHeaps();
    };
/*
    struct LiveCommandBuffer
    {
      vector<std::shared_ptr<backend::SemaphoreImpl>> wait;
      vector<std::shared_ptr<backend::CommandBufferImpl>> lists;
      vector<std::shared_ptr<backend::SemaphoreImpl>> signal;
      std::shared_ptr<backend::FenceImpl> fence;
      std::shared_ptr<vector<IntermediateList>> intermediateLists;
    };*/



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