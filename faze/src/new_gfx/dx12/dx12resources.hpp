#pragma once
#if defined(FAZE_PLATFORM_WINDOWS)
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"
#include "faze/src/new_gfx/common/commandpackets.hpp"

#include "core/src/system/MemoryPools.hpp"
#include "core/src/system/MovePtr.hpp"

#include "dx12.hpp"

#include "faze/src/new_gfx/dx12/util/ShaderStorage.hpp"

#include "faze/src/new_gfx/definitions.hpp"
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
#include <DXGIDebug.h>
#endif

#include <memory>

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

    struct UploadBlock
    {
      uint8_t* m_data;
      PageBlock block;

      uint8_t* data()
      {
        return m_data + block.offset;
      }

      size_t size()
      {
        return block.size;
      }

      explicit operator bool() const
      {
        return m_data != nullptr;
      }
    };

    // TODO: protect with mutex
    // accessed in dx12commandbuffer
    class DX12UploadHeap
    {
      FixedSizeAllocator allocator;
      ComPtr<ID3D12Resource> resource;
      unsigned fixedSize = 1;
      unsigned size = 1;

      uint8_t* data = nullptr;
    public:
      DX12UploadHeap() : allocator(1, 1) {}
      DX12UploadHeap(ID3D12Device* device, unsigned allocationSize, unsigned allocationCount)
        : allocator(allocationSize, allocationCount)
        , fixedSize(allocationSize)
        , size(allocationSize*allocationCount)
      {
        {
          D3D12_RESOURCE_DESC dxdesc{};

          dxdesc.Width = allocationSize * allocationCount;
          dxdesc.Height = 1;
          dxdesc.DepthOrArraySize = 1;
          dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
          dxdesc.Format = DXGI_FORMAT_UNKNOWN;
          dxdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
          dxdesc.MipLevels = 1;
          dxdesc.SampleDesc.Count = 1;
          dxdesc.Flags = D3D12_RESOURCE_FLAG_NONE;

          D3D12_HEAP_PROPERTIES heap{};
          heap.Type = D3D12_HEAP_TYPE_UPLOAD;
          heap.CreationNodeMask = 0;

          FAZE_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &dxdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));
        }

        D3D12_RANGE range{};
        range.End = allocationSize * allocationCount;

        FAZE_CHECK_HR(resource->Map(0, &range, reinterpret_cast<void**>(&data)));
      }

      UploadBlock allocate(size_t bytes)
      {
        auto dip = allocator.allocate(bytes);
        F_ASSERT(dip.offset != -1, "No descriptors left, make bigger Staging :) %d", size);
        return UploadBlock{ data, dip };
      }

      void release(UploadBlock desc)
      {
        allocator.release(desc.block);
      }
    };

    class DX12Fence : public FenceImpl, public SemaphoreImpl
    {
    public:
      ComPtr<ID3D12Fence> fence = nullptr;
      std::shared_ptr<HANDLE> handle = nullptr;
      std::shared_ptr<uint64_t> value = nullptr;

      DX12Fence()
      {}

      DX12Fence(ComPtr<ID3D12Fence> fence)
        : fence(fence)
        , handle(std::shared_ptr<HANDLE>(new HANDLE(CreateEventExA(nullptr, nullptr, 0, EVENT_ALL_ACCESS)), [](HANDLE* ptr)
      {
        CloseHandle(*ptr);
        delete ptr;
      }))
        , value(std::make_shared<uint64_t>(0))
      {
      }

      uint64_t start()
      {
        return ++(*value);
      }

      bool hasCompleted()
      {
        auto val = fence->GetCompletedValue();
        return val == *value;
      }

      void waitTillReady(DWORD dwMilliseconds = INFINITE)
      {
        if (hasCompleted())
          return;
        fence->SetEventOnCompletion(*value, *handle);
        DWORD result = WaitForSingleObject(*handle, dwMilliseconds);
        F_ASSERT(WAIT_OBJECT_0 == result, "Fence wait failed.");
      }
    };

    class DX12CommandBuffer
    {
      ComPtr<ID3D12GraphicsCommandList> commandList;
      ComPtr<ID3D12CommandAllocator> commandListAllocator;
      bool closedList = false;
    public:
      //DX12CommandBuffer() {}
      DX12CommandBuffer(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandAllocator> commandListAllocator)
        : commandList(commandList)
        , commandListAllocator(commandListAllocator)
      {
      }

      DX12CommandBuffer(DX12CommandBuffer&& other) = default;
      DX12CommandBuffer(const DX12CommandBuffer& other) = delete;

      DX12CommandBuffer& operator=(DX12CommandBuffer&& other) = default;
      DX12CommandBuffer& operator=(const DX12CommandBuffer& other) = delete;

      ID3D12GraphicsCommandList* list()
      {
        return commandList.Get();
      }

      void closeList()
      {
        commandList->Close();
        closedList = true;
      }

      void resetList()
      {
        if (!closedList)
          commandList->Close();
        commandListAllocator->Reset();
        commandList->Reset(commandListAllocator.Get(), nullptr);
        closedList = false;
      }

      bool closed() const
      {
        return closedList;
      }
    };

    class DX12CommandList : public CommandBufferImpl
    {
      std::shared_ptr<DX12CommandBuffer> m_buffer;
      std::shared_ptr<DX12UploadHeap> m_constants;

      struct FreeableResources
      {
        vector<UploadBlock> uploadBlocks;
      };

      std::shared_ptr<FreeableResources> m_freeResources;
    public:
      DX12CommandList(std::shared_ptr<DX12CommandBuffer> buffer, std::shared_ptr<DX12UploadHeap> constants)
        : m_buffer(buffer)
        , m_constants(constants)
      {
        m_buffer->resetList();

        std::weak_ptr<DX12UploadHeap> consts = m_constants;

        m_freeResources = std::shared_ptr<FreeableResources>(new FreeableResources, [consts](FreeableResources* ptr)
        {
          if (auto constants = consts.lock())
          {
            for (auto&& it : ptr->uploadBlocks)
            {
              constants->release(it);
            }
          }

          delete ptr;
        });
      }

      void fillWith(std::shared_ptr<prototypes::DeviceImpl>, backend::IntermediateList&) override;

      bool closed() const
      {
        return m_buffer->closed();
      }

      ID3D12GraphicsCommandList* list()
      {
        return m_buffer->list();
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

      int getCurrentPresentableImageIndex() override
      {
        return m_backbufferIndex;
      }

      std::shared_ptr<SemaphoreImpl> acquireSemaphore() override
      {
        return nullptr;
      }

      std::shared_ptr<backend::SemaphoreImpl> renderSemaphore() override
      {
        return nullptr;
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

    struct DX12TextureState
    {
      vector<D3D12_RESOURCE_STATES> flags; // subresource count
    };

    class DX12Texture : public prototypes::TextureImpl
    {
    private:
      ID3D12Resource* resource = nullptr;
      std::shared_ptr<DX12TextureState> statePtr = nullptr;

    public:
      DX12Texture()
      {}
      DX12Texture(ID3D12Resource* resource, std::shared_ptr<DX12TextureState> state)
        : resource(resource)
        , statePtr(state)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }

      std::shared_ptr<DX12TextureState> state()
      {
        return statePtr;
      }

      backend::TrackedState dependency() override
      {
        backend::TrackedState state{};
        state.resPtr = reinterpret_cast<size_t>(resource);
        state.statePtr = reinterpret_cast<size_t>(statePtr.get());
        state.additionalInfo = 0;
        return state;
      }
    };

    class DX12TextureView : public prototypes::TextureViewImpl
    {
    private:
      DX12Descriptor resource;
      SubresourceRange subResourceRange;
    public:
      DX12TextureView()
      {}
      DX12TextureView(DX12Descriptor resource, SubresourceRange subResourceRange)
        : resource(resource)
        , subResourceRange(subResourceRange)
      {}
      DX12Descriptor native()
      {
        return resource;
      }

      SubresourceRange range()
      {
        return subResourceRange;
      }

      backend::RawView view() override
      {
        backend::RawView view;
        view.view = resource.cpu.ptr;
        return view;
      }
    };

    class DX12Buffer : public prototypes::BufferImpl
    {
    private:
      ID3D12Resource* resource;
      std::shared_ptr<D3D12_RESOURCE_STATES> statePtr = nullptr;

    public:
      DX12Buffer()
      {}
      DX12Buffer(ID3D12Resource* resource, std::shared_ptr<D3D12_RESOURCE_STATES> state)
        : resource(resource)
        , statePtr(state)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }
      std::shared_ptr<D3D12_RESOURCE_STATES> state()
      {
        return statePtr;
      }
      backend::TrackedState dependency() override
      {
        backend::TrackedState state{};
        state.resPtr = reinterpret_cast<size_t>(resource);
        state.statePtr = reinterpret_cast<size_t>(statePtr.get());
        state.additionalInfo = 0;
        return state;
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

      backend::RawView view() override
      {
        backend::RawView view;
        view.view = resource.cpu.ptr;
        return view;
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

    class DX12Renderpass : public prototypes::RenderpassImpl
    {
    private:
      int _unused = -1;
    public:
      DX12Renderpass() {}
    };

    class DX12Pipeline : public prototypes::PipelineImpl
    {
    public:
      ComPtr<ID3D12PipelineState> pipeline;
      ComPtr<ID3D12RootSignature> root;
      DX12Pipeline()
        : pipeline(nullptr)
        , root(nullptr)
      {
      }
      DX12Pipeline(ComPtr<ID3D12PipelineState> pipeline, ComPtr<ID3D12RootSignature> root)
        : pipeline(pipeline)
        , root(root)
      {
      }
    };

    class DX12Device : public prototypes::DeviceImpl
    {
    private:
      GpuInfo m_info;
      ComPtr<ID3D12Device> m_device;
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
      ComPtr<IDXGIDebug1> m_debug;
#endif
      ComPtr<IDXGIFactory4> m_factory;

      FileSystem& m_fs;
      DX12ShaderStorage m_shaders;

      UINT m_nodeMask;
      ComPtr<ID3D12CommandQueue> m_graphicsQueue;
      ComPtr<ID3D12CommandQueue> m_dmaQueue;
      ComPtr<ID3D12CommandQueue> m_computeQueue;
      DX12Fence m_deviceFence;

      StagingDescriptorHeap m_generics;
      StagingDescriptorHeap m_samplers;
      StagingDescriptorHeap m_rtvs;
      StagingDescriptorHeap m_dsvs;

      Rabbitpool2<DX12CommandBuffer> m_copyListPool;
      Rabbitpool2<DX12CommandBuffer> m_computeListPool;
      Rabbitpool2<DX12CommandBuffer> m_graphicsListPool;
      Rabbitpool2<DX12Fence> m_fencePool;

      std::shared_ptr<DX12UploadHeap> m_constantsUpload;

    public:
      DX12Device(GpuInfo info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory4> factory, FileSystem& fs);
      ~DX12Device();

      D3D12_RESOURCE_DESC fillPlacedBufferInfo(ResourceDescriptor descriptor);
      D3D12_RESOURCE_DESC fillPlacedTextureInfo(ResourceDescriptor descriptor);

      void updatePipeline(GraphicsPipeline& pipeline, gfxpacket::Subpass& subpass);
      void updatePipeline(ComputePipeline& pipeline);

      // impl
      std::shared_ptr<prototypes::PipelineImpl> createPipeline() override;

      std::shared_ptr<prototypes::SwapchainImpl> createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int bufferCount);
      void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, PresentMode mode, FormatType format, int bufferCount) override;
      void destroySwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      vector<std::shared_ptr<prototypes::TextureImpl>> getSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;

      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      std::shared_ptr<prototypes::RenderpassImpl> createRenderpass() override;

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
      DX12CommandBuffer createList(D3D12_COMMAND_LIST_TYPE type);
      DX12Fence         createNativeFence();
      std::shared_ptr<CommandBufferImpl> createDMAList() override;
      std::shared_ptr<CommandBufferImpl> createComputeList() override;
      std::shared_ptr<CommandBufferImpl> createGraphicsList() override;
      std::shared_ptr<SemaphoreImpl>     createSemaphore() override;
      std::shared_ptr<FenceImpl>         createFence() override;

      void submit(
        ComPtr<ID3D12CommandQueue> queue,
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence);

      void submitDMA(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence) override;

      void submitCompute(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence) override;

      void submitGraphics(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence) override;

      void waitFence(std::shared_ptr<FenceImpl>     fence) override;
      bool checkFence(std::shared_ptr<FenceImpl>    fence) override;
      void present(std::shared_ptr<prototypes::SwapchainImpl> swapchain, std::shared_ptr<SemaphoreImpl> renderingFinished) override;
    };

    class DX12Subsystem : public prototypes::SubsystemImpl
    {
      vector<GpuInfo> infos;
      ComPtr<IDXGIFactory4> pFactory;
      std::vector<ComPtr<IDXGIAdapter3>> vAdapters;
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