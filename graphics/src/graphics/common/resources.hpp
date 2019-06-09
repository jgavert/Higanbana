#pragma once

#include "graphics/common/backend.hpp"

//#include "graphics/common/pipeline.hpp"
//#include "core/filesystem/filesystem.hpp"
//#include "core/system/PageAllocator.hpp"
#include "core/datastructures/proxy.hpp"
//#include "graphics/common/intermediatelist.hpp"

#include <core/system/SequenceTracker.hpp>
#include <graphics/common/handle.hpp>

//#include <string>
//#include <atomic>
#include <memory>


namespace faze
{
  enum class GraphicsApi
  {
    Vulkan,
    DX12,
	  All
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
    Amd, // = 4098,
    Nvidia, // = 4318,
    Intel, // = 32902
    Unknown
  };

  struct GpuInfo
  {
    int id;
    std::string name;
    int64_t memory;
    VendorID vendor;
    DeviceType type;
    bool canPresent;
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

#if defined(FAZE_PLATFORM_WINDOWS)
    struct SharedHandle
    {
      void* handle;
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

    struct LiveCommandBuffer
    {
      vector<std::shared_ptr<backend::SemaphoreImpl>> wait;
      vector<std::shared_ptr<backend::CommandBufferImpl>> lists;
      vector<std::shared_ptr<backend::SemaphoreImpl>> signal;
      std::shared_ptr<backend::FenceImpl> fence;
      std::shared_ptr<vector<IntermediateList>> intermediateLists;
    };

    struct LiveCommandBuffer2
    {
      int deviceID;
      SeqNum started;
      vector<std::shared_ptr<backend::SemaphoreImpl>> wait;
      vector<std::shared_ptr<backend::CommandBufferImpl>> lists;
      vector<std::shared_ptr<backend::SemaphoreImpl>> signal;
      std::shared_ptr<backend::FenceImpl> fence;
    };
  }
}