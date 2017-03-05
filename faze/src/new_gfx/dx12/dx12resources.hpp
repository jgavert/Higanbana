#pragma once
#if defined(FAZE_PLATFORM_WINDOWS)
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"

#include "dx12.hpp"

namespace faze
{
  namespace backend
  {
    struct DX12GPUDescriptor
    {
      D3D12_CPU_DESCRIPTOR_HANDLE cpu;
      D3D12_GPU_DESCRIPTOR_HANDLE gpu;
    };

    struct DX12Descriptor
    {
      D3D12_CPU_DESCRIPTOR_HANDLE cpu;
      D3D12_DESCRIPTOR_HEAP_TYPE type;
      RangeBlock block;
    };

    struct DX12DynamicDescriptorRange
    {
      D3D12_GPU_DESCRIPTOR_HANDLE gpu;
      D3D12_CPU_DESCRIPTOR_HANDLE cpu;
      UINT increment = 0;
      int size = -1;

      DX12GPUDescriptor offset(int index)
      {
        F_ASSERT(index < size && index >= 0, "Invalid index %d", index);
        DX12GPUDescriptor desc{};
        desc.cpu.ptr = cpu.ptr + static_cast<size_t>(index) * increment;
        desc.gpu.ptr = gpu.ptr + static_cast<size_t>(index) * increment;
        return desc;
      }
    };

    class LinearDescriptorHeap
    {
      LinearAllocator allocator;
      ComPtr<ID3D12DescriptorHeap> heap;
      D3D12_DESCRIPTOR_HEAP_TYPE type;
      D3D12_CPU_DESCRIPTOR_HANDLE cpuStart;
      D3D12_GPU_DESCRIPTOR_HANDLE gpuStart;
      UINT increment;

      int size = -1;
    public:
      LinearDescriptorHeap() {}
      LinearDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, int count)
        : allocator(count)
        , type(type)
        , size(count)
      {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = type;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;
        desc.NumDescriptors = static_cast<UINT>(count);

        FAZE_CHECK_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));

        cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
        gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
        increment = device->GetDescriptorHandleIncrementSize(type);
      }

      DX12DynamicDescriptorRange allocate(int value)
      {
        auto offset = allocator.allocate(value, 1);
        F_ASSERT(offset != -1, "No descriptors left, make bigger Staging :) %d type: %d", size, static_cast<int>(type));
        DX12DynamicDescriptorRange desc{};
        desc.cpu.ptr = cpuStart.ptr + increment * offset;
        desc.gpu.ptr = gpuStart.ptr + increment * offset;
        desc.increment = increment;
        desc.size = value;
        return desc;
      }

      void reset()
      {
        allocator.reset();
      }
    };

    class StagingDescriptorHeap
    {
      RangeBlockAllocator allocator;
      ComPtr<ID3D12DescriptorHeap> heap;
      D3D12_DESCRIPTOR_HEAP_TYPE type;
      D3D12_CPU_DESCRIPTOR_HANDLE start;
      UINT increment;

      int size = -1;
    public:
      StagingDescriptorHeap() {}
      StagingDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, int count)
        : allocator(count)
        , type(type)
        , size(count)
      {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = type;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;
        desc.NumDescriptors = static_cast<UINT>(count);

        FAZE_CHECK_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));

        start = heap->GetCPUDescriptorHandleForHeapStart();
        increment = device->GetDescriptorHandleIncrementSize(type);
      }

      DX12Descriptor allocate()
      {
        auto dip = allocator.allocate(1);
        F_ASSERT(dip.offset != -1, "No descriptors left, make bigger Staging :) %d type: %d", size, static_cast<int>(type));
        DX12Descriptor desc{};
        desc.block = dip;
        desc.type = type;
        desc.cpu.ptr = start.ptr + increment * dip.offset;
        return desc;
      }

      void release(DX12Descriptor desc)
      {
        allocator.release(desc.block);
      }
    };

    class DX12Fence : public prototypes::FenceImpl, public prototypes::SemaphoreImpl
    {
    public:
      ComPtr<ID3D12Fence> fence = nullptr;
      std::shared_ptr<HANDLE> handle = nullptr;
      uint64_t value = 0;

      DX12Fence()
      {}

      DX12Fence(ComPtr<ID3D12Fence> fence)
        : fence(fence)
        , handle(std::shared_ptr<HANDLE>(new HANDLE(CreateEventExA(nullptr, nullptr, 0, EVENT_ALL_ACCESS)), [](HANDLE* ptr)
            {
              CloseHandle(*ptr);
              delete ptr;
            }))
      {
      }
      
      uint64_t start()
      {
        return ++value;
      }

      bool hasCompleted()
      {
        auto val = fence->GetCompletedValue();
        return val == value;
      }

      void waitTillReady(DWORD dwMilliseconds = INFINITE)
      {
        if (hasCompleted())
          return;
        fence->SetEventOnCompletion(value, *handle);
        DWORD result = WaitForSingleObject(*handle, dwMilliseconds);
        F_ASSERT(WAIT_OBJECT_0 == result, "Fence wait failed.");
      }
    };


    class DX12CommandBuffer : public prototypes::CommandBufferImpl
    {
      ComPtr<ID3D12GraphicsCommandList> commandList;
      ComPtr<ID3D12CommandAllocator> commandListAllocator;
      bool closedList = false;
      std::shared_ptr<LinearDescriptorHeap> resources;
      std::shared_ptr<LinearDescriptorHeap> samplers;
      // needs the dynamicheaps
    public:
      DX12CommandBuffer() {}
      DX12CommandBuffer(ComPtr<ID3D12GraphicsCommandList> commandList,
        ComPtr<ID3D12CommandAllocator> commandListAllocator,
        std::shared_ptr<LinearDescriptorHeap> resources,
        std::shared_ptr<LinearDescriptorHeap> samplers)
        : commandList(commandList)
        , commandListAllocator(commandListAllocator)
        , resources(resources)
        , samplers(samplers)
      {}

      void fillWith(backend::IntermediateList& )
      {
        // TODO: move this function somewhere where we have space to actually implement this.
      }

      ID3D12GraphicsCommandList* list()
      {
        return commandList.Get();
      }

      void reset()
      {
        commandListAllocator->Reset();
        commandList->Reset(commandListAllocator.Get(), nullptr);
        closedList = false;
        resources->reset();
        samplers->reset();
      }

      bool closed()
      {
        return closedList;
      }
    };

    // implementations
    class DX12GraphicsSurface : public prototypes::GraphicsSurfaceImpl
    {
    private:
      HWND hwnd;
      HINSTANCE instance;

    public:
      DX12GraphicsSurface()
      {}
      DX12GraphicsSurface(HWND hwnd, HINSTANCE instance)
        : hwnd(hwnd)
        , instance(instance)
      {}
      HWND native()
      {
        return hwnd;
      }
    };

    class DX12Swapchain : public prototypes::SwapchainImpl
    {
    private:
      D3D12Swapchain* m_resource;
      DX12GraphicsSurface m_surface;
      int m_backbufferIndex;

      struct Desc
      {
        int width = 0;
        int height = 0;
        int buffers = 0;
        FormatType format = FormatType::Unknown;
        PresentMode mode = PresentMode::Unknown;
      } m_desc;

    public:
      DX12Swapchain()
      {}
      DX12Swapchain(D3D12Swapchain* resource, DX12GraphicsSurface surface)
        : m_resource(resource)
        , m_surface(surface)
      {}

      ResourceDescriptor desc() override
      {
        return ResourceDescriptor()
          .setWidth(m_desc.width)
          .setHeight(m_desc.height)
          .setFormat(m_desc.format)
          .setUsage(ResourceUsage::RenderTarget)
          .setDimension(FormatDimension::Texture2D)
          .setMiplevels(1)
          .setArraySize(1)
          .setName("Swapchain Image")
          .setDepth(1);
      }

      void setBufferMetadata(int x, int y, int count, FormatType format, PresentMode mode)
      {
        m_desc.width = x;
        m_desc.height = y;
        m_desc.buffers = count;
        m_desc.format = format;
        m_desc.mode = mode;
      }

      Desc getDesc()
      {
        return m_desc;
      }

      D3D12Swapchain* native()
      {
        return m_resource;
      }

      DX12GraphicsSurface& surface()
      {
        return m_surface;
      }

      void setBackbufferIndex(int index)
      {
        m_backbufferIndex = index;
      }
    };

    class DX12Texture : public prototypes::TextureImpl
    {
    private:
      ID3D12Resource* resource;

    public:
      DX12Texture()
      {}
      DX12Texture(ID3D12Resource* resource)
        : resource(resource)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }
    };

    class DX12TextureView : public prototypes::TextureViewImpl
    {
    private:
      DX12Descriptor resource;

    public:
      DX12TextureView()
      {}
      DX12TextureView(DX12Descriptor resource)
        : resource(resource)
      {}
      DX12Descriptor native()
      {
        return resource;
      }
    };

    class DX12Buffer : public prototypes::BufferImpl
    {
    private:
      ID3D12Resource* resource;

    public:
      DX12Buffer()
      {}
      DX12Buffer(ID3D12Resource* resource)
        : resource(resource)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }
    };

    class DX12BufferView : public prototypes::BufferViewImpl
    {
    private:
      DX12Descriptor resource;

    public:
      DX12BufferView()
      {}
      DX12BufferView(DX12Descriptor resource)
        : resource(resource)
      {}
      DX12Descriptor native()
      {
        return resource;
      }
    };

    class DX12Heap : public prototypes::HeapImpl
    {
    private:
      ID3D12Heap* heap;

    public:
      DX12Heap()
      {}
      DX12Heap(ID3D12Heap* heap)
        : heap(heap)
      {}
      ID3D12Heap* native()
      {
        return heap;
      }
    };

    class DX12Device : public prototypes::DeviceImpl
    {
    private:
      GpuInfo m_info;
      ComPtr<ID3D12Device> m_device;
      UINT m_nodeMask;
      ComPtr<ID3D12CommandQueue> m_graphicsQueue;
      ComPtr<ID3D12CommandQueue> m_dmaQueue;
      ComPtr<ID3D12CommandQueue> m_computeQueue;
      DX12Fence m_deviceFence;

      StagingDescriptorHeap m_generics;
      StagingDescriptorHeap m_samplers;
      StagingDescriptorHeap m_rtvs;
      StagingDescriptorHeap m_dsvs;
    public:
      DX12Device(GpuInfo info, ComPtr<ID3D12Device> device);
      ~DX12Device();

      D3D12_RESOURCE_DESC fillPlacedBufferInfo(ResourceDescriptor descriptor);
      D3D12_RESOURCE_DESC fillPlacedTextureInfo(ResourceDescriptor descriptor);

      // impl
      std::shared_ptr<prototypes::SwapchainImpl> createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int bufferCount);
      void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, PresentMode mode, FormatType format, int bufferCount) override;
      void destroySwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      vector<std::shared_ptr<prototypes::TextureImpl>> getSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      int acquirePresentableImageIndex(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;

      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      GpuHeap createHeap(HeapDescriptor desc) override;
      void destroyHeap(GpuHeap heap) override;

      std::shared_ptr<prototypes::BufferImpl> createBuffer(HeapAllocation allocation, ResourceDescriptor& desc) override;
      void destroyBuffer(std::shared_ptr<prototypes::BufferImpl> buffer) override;

      std::shared_ptr<prototypes::BufferViewImpl> createBufferView(std::shared_ptr<prototypes::BufferImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void destroyBufferView(std::shared_ptr<prototypes::BufferViewImpl> buffer) override;

      std::shared_ptr<prototypes::TextureImpl> createTexture(HeapAllocation allocation, ResourceDescriptor& desc) override;
      void destroyTexture(std::shared_ptr<prototypes::TextureImpl> buffer) override;

      std::shared_ptr<prototypes::TextureViewImpl> createTextureView(std::shared_ptr<prototypes::TextureImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void destroyTextureView(std::shared_ptr<prototypes::TextureViewImpl> buffer) override;

      // commandlist things and gpu-cpu/gpu-gpu synchronization primitives
      std::shared_ptr<prototypes::CommandBufferImpl> createList(D3D12_COMMAND_LIST_TYPE type);
      std::shared_ptr<prototypes::CommandBufferImpl> createDMAList() override;
      std::shared_ptr<prototypes::CommandBufferImpl> createComputeList() override;
      std::shared_ptr<prototypes::CommandBufferImpl> createGraphicsList() override;
      void resetList(std::shared_ptr<prototypes::CommandBufferImpl> list) override;
      std::shared_ptr<prototypes::SemaphoreImpl>     createSemaphore() override;
      std::shared_ptr<prototypes::FenceImpl>         createFence() override;

      void submit(
        ComPtr<ID3D12CommandQueue> queue,
        MemView<std::shared_ptr<prototypes::CommandBufferImpl>> lists,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<prototypes::FenceImpl>>         fence);

      void submitDMA(
        MemView<std::shared_ptr<prototypes::CommandBufferImpl>> lists,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<prototypes::FenceImpl>>         fence) override;

      void submitCompute(
        MemView<std::shared_ptr<prototypes::CommandBufferImpl>> lists,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<prototypes::FenceImpl>>         fence) override;

      void submitGraphics(
        MemView<std::shared_ptr<prototypes::CommandBufferImpl>> lists,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<prototypes::SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<prototypes::FenceImpl>>         fence) override;

      void waitFence(std::shared_ptr<prototypes::FenceImpl>     fence) override;
      bool checkFence(std::shared_ptr<prototypes::FenceImpl>    fence) override;
    };

    class DX12Subsystem : public prototypes::SubsystemImpl
    {
      vector<GpuInfo> infos;
      ComPtr<IDXGIFactory5> pFactory;
      std::vector<IDXGIAdapter3*> vAdapters;
    public:
      DX12Subsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi() override;
      vector<GpuInfo> availableGpus() override;
      GpuDevice createGpuDevice(FileSystem& fs, GpuInfo gpu) override;
      GraphicsSurface createSurface(Window& window) override;
    };
  }
}
#endif