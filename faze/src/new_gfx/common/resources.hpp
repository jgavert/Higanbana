#pragma once

#include "backend.hpp"

#include "heap_descriptor.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/system/PageAllocator.hpp"
#include "core/src/datastructures/proxy.hpp"
#include "intermediatelist.hpp"

#include <string>
#include <atomic>

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

  enum class VendorID
  {
      Amd, // = 4098,
      Nvidia, // = 4318,
      Intel, // implemented but lol number 
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
  };

  struct MemoryRequirements
  {
    size_t alignment;
    size_t bytes;
    int64_t heapType;
  };

  struct GpuHeapAllocation
  {
    int index;
    int alignment;
    int64_t heapType;
    PageBlock block;

    bool valid() { return alignment != -1 && index != -1; }
  };

  class Buffer;
  class Texture;

  class BufferSRV;
  class BufferUAV;
  class TextureSRV;
  class TextureUAV;
  class TextureRTV;
  class TextureDSV;

  class Swapchain;
  class GraphicsSurface;
  class Window;
  class CommandGraph;

  namespace backend
  {
    class FenceImpl;
    class SemaphoreImpl;
    class CommandBufferImpl;
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

    struct GpuHeap
    {
      std::shared_ptr<prototypes::HeapImpl> impl;
      std::shared_ptr<HeapDescriptor> desc;

      GpuHeap(std::shared_ptr<prototypes::HeapImpl> impl, HeapDescriptor desc)
        : impl(impl)
        , desc(std::make_shared<HeapDescriptor>(desc))
      {
      }
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
        int index;
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
      const int64_t m_minimumHeapSize = 16 * 1024 * 1024;
    public:

      HeapAllocation allocate(prototypes::DeviceImpl* device, MemoryRequirements requirements);
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

    struct DeviceData : std::enable_shared_from_this<DeviceData>
    {
      std::shared_ptr<prototypes::DeviceImpl> m_impl;
      HeapManager m_heaps;

      std::shared_ptr<ResourceTracker<prototypes::BufferImpl>> m_trackerbuffers;
      std::shared_ptr<ResourceTracker<prototypes::TextureImpl>> m_trackertextures;
      std::shared_ptr<ResourceTracker<prototypes::SwapchainImpl>> m_trackerswapchains;
      std::shared_ptr<ResourceTracker<prototypes::BufferViewImpl>> m_trackerbufferViews;
      std::shared_ptr<ResourceTracker<prototypes::TextureViewImpl>> m_trackertextureViews;

      std::shared_ptr<std::atomic<int64_t>> m_idGenerator;
      deque<LiveCommandBuffer> m_buffers;

      int64_t newId() { return (*m_idGenerator)++; }

      DeviceData(std::shared_ptr<prototypes::DeviceImpl> impl);
      DeviceData(DeviceData&& data) = default;
      DeviceData(const DeviceData& data) = delete;
      DeviceData& operator=(DeviceData&& data) = default;
      DeviceData& operator=(const DeviceData& data) = delete;
      ~DeviceData();

      void checkCompletedLists();
      void gc();
      void waitGpuIdle();
      Swapchain createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int bufferCount);
      void adjustSwapchain(Swapchain& swapchain, PresentMode mode, FormatType format, int bufferCount);
      TextureRTV acquirePresentableImage(Swapchain& swapchain);

      Buffer createBuffer(ResourceDescriptor desc);
      Texture createTexture(ResourceDescriptor desc);

      BufferSRV createBufferSRV(Buffer texture, ShaderViewDescriptor viewDesc);
      BufferUAV createBufferUAV(Buffer texture, ShaderViewDescriptor viewDesc);
      TextureSRV createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureUAV createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureRTV createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureDSV createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc);
      // commandgraph
      void submit(Swapchain& swapchain, CommandGraph graph);
      void present(Swapchain& swapchain);
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
      GraphicsSurface createSurface(Window& window);
    };
  }
}