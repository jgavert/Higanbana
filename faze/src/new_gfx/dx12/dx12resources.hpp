#pragma once
#if defined(FAZE_PLATFORM_WINDOWS)
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"
#include "faze/src/new_gfx/common/commandpackets.hpp"

#include "core/src/system/MemoryPools.hpp"
#include "core/src/system/MovePtr.hpp"
#include "core/src/system/SequenceTracker.hpp"

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
    class DX12DependencySolver;
    class DX12Device;

    struct DX12GPUDescriptor
    {
      D3D12_CPU_DESCRIPTOR_HANDLE cpu;
      D3D12_GPU_DESCRIPTOR_HANDLE gpu;
    };

    struct DX12CPUDescriptor
    {
      D3D12_CPU_DESCRIPTOR_HANDLE cpu;
      D3D12_DESCRIPTOR_HEAP_TYPE type;
      RangeBlock block;
    };

    struct DynamicDescriptorBlock
    {
      D3D12_CPU_DESCRIPTOR_HANDLE baseCpuHandle;
      D3D12_GPU_DESCRIPTOR_HANDLE baseGpuHandle;
      UINT increment = 0;
      PageBlock block;

      DX12GPUDescriptor offset(int index) const
      {
        F_ASSERT(index < block.size && index >= 0, "Invalid index %d", index);
        DX12GPUDescriptor desc{};
        desc.cpu.ptr = baseCpuHandle.ptr + static_cast<size_t>(index + block.offset) * increment;
        desc.gpu.ptr = baseGpuHandle.ptr + static_cast<size_t>(index + block.offset) * increment;
        return desc;
      }

      operator bool() const
      {
        return block.valid();
      }

      size_t size() const
      {
        return block.size;
      }
    };

    class LinearDescriptorAllocator
    {
      LinearAllocator allocator;
      DynamicDescriptorBlock block;

    public:
      LinearDescriptorAllocator() {}
      LinearDescriptorAllocator(DynamicDescriptorBlock block)
        : allocator(block.size())
        , block(block)
      {
      }

      DynamicDescriptorBlock allocate(size_t bytes)
      {
        auto offset = allocator.allocate(bytes);
        if (offset < 0)
          return DynamicDescriptorBlock{ 0, 0, 0, PageBlock{} };

        DynamicDescriptorBlock b = block;
        b.block.offset += offset;
        b.block.size = bytes;
        return b;
      }
    };

    class DX12DynamicDescriptorHeap
    {
      FixedSizeAllocator allocator;
      ComPtr<ID3D12DescriptorHeap> heap;
      D3D12_DESCRIPTOR_HEAP_TYPE type;
      DynamicDescriptorBlock baseRange;
      int size = -1;
    public:
      DX12DynamicDescriptorHeap()
        : allocator(0, 0)
      {}
      DX12DynamicDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, int blockSize, int blockCount)
        : allocator(blockSize, blockCount)
        , type(type)
        , size(blockSize*blockCount)
      {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = type;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;
        desc.NumDescriptors = static_cast<UINT>(size);

        FAZE_CHECK_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

        baseRange.baseCpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        baseRange.baseGpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
        baseRange.increment = device->GetDescriptorHandleIncrementSize(type);
      }

      DynamicDescriptorBlock allocate(int value)
      {
        auto offset = allocator.allocate(value);
        F_ASSERT(offset.offset != -1, "No descriptors left, make bigger Staging :) %d type: %d", size, static_cast<int>(type));
        DynamicDescriptorBlock desc = baseRange;
        desc.block = offset;
        return desc;
      }

      void release(DynamicDescriptorBlock range)
      {
        F_ASSERT(range.block.offset != -1, "halp");
        allocator.release(range.block);
      }

      ID3D12DescriptorHeap* native()
      {
        return heap.Get();
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

      DX12CPUDescriptor allocate()
      {
        auto dip = allocator.allocate(1);
        F_ASSERT(dip.offset != -1, "No descriptors left, make bigger Staging :) %d type: %d", size, static_cast<int>(type));
        DX12CPUDescriptor desc{};
        desc.block = dip;
        desc.type = type;
        desc.cpu.ptr = start.ptr + increment * dip.offset;
        return desc;
      }

      void release(DX12CPUDescriptor desc)
      {
        allocator.release(desc.block);
      }
    };

    struct UploadBlock
    {
      uint8_t* m_data;
      D3D12_GPU_VIRTUAL_ADDRESS m_resourceAddress;
      PageBlock block;

      uint8_t* data()
      {
        return m_data + block.offset;
      }

      D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress()
      {
        return m_resourceAddress + block.offset;
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

    class UploadLinearAllocator
    {
      LinearAllocator allocator;
      UploadBlock block;

    public:
      UploadLinearAllocator() {}
      UploadLinearAllocator(UploadBlock block)
        : allocator(block.size())
        , block(block)
      {
      }

      UploadBlock allocate(size_t bytes, size_t alignment)
      {
        auto offset = allocator.allocate(bytes, alignment);
        if (offset < 0)
          return UploadBlock{ nullptr, 0, PageBlock{} };

        UploadBlock b = block;
        b.block.offset += offset;
        b.block.size = roundUpMultipleInt(bytes, alignment);
        return b;
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
      D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;
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

          gpuAddr = resource->GetGPUVirtualAddress();
        }

        D3D12_RANGE range{};
        range.End = allocationSize * allocationCount;

        FAZE_CHECK_HR(resource->Map(0, &range, reinterpret_cast<void**>(&data)));
      }

      UploadBlock allocate(size_t bytes)
      {
        auto dip = allocator.allocate(bytes);
        F_ASSERT(dip.offset != -1, "No space left, make bigger DX12UploadHeap :) %d", size);
        return UploadBlock{ data, gpuAddr,  dip };
      }

      void release(UploadBlock desc)
      {
        allocator.release(desc.block);
      }

      ID3D12Resource* native()
      {
        return resource.Get();
      }
    };

    struct ReadbackBlock
    {
      size_t offset;
      size_t m_size;

      size_t size() const
      {
        return m_size;
      }
      explicit operator bool() const
      {
        return size() > 0;
      }
    };

    struct DX12Query
    {
        unsigned beginIndex;
        unsigned endIndex;
    };

    struct QueryBracket
    {
        DX12Query query;
        std::string name;
    };

    class DX12QueryHeap
    {
        LinearAllocator allocator;
        ComPtr<ID3D12QueryHeap> heap;
        int m_size = 0;
        uint64_t m_gpuTicksPerSecond = 0;
    public:
        DX12QueryHeap() : allocator(1) {}
        DX12QueryHeap(ID3D12Device* device, ID3D12CommandQueue* queue, unsigned counterCount)
            : allocator(counterCount)
            , m_size(counterCount)
        {

            static int queryHeapCount = 0;

            D3D12_QUERY_HEAP_DESC desc{};
            desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            desc.Count = counterCount;
            desc.NodeMask = 0;

            FAZE_CHECK_HR(device->CreateQueryHeap(&desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));
            auto name = s2ws("QueryHeap" + std::to_string(queryHeapCount));
            heap->SetName(name.c_str());

            FAZE_CHECK_HR(queue->GetTimestampFrequency(&m_gpuTicksPerSecond));
        }

        uint64_t getGpuTicksPerSecond() const
        {
            return m_gpuTicksPerSecond;
        }

        void reset()
        {
            allocator = LinearAllocator(m_size);
        }

        DX12Query allocate()
        {
            auto dip = allocator.allocate(2);
            F_ASSERT(dip != -1, "Queryheap out of queries.");
            return DX12Query{ static_cast<unsigned>(dip),  static_cast<unsigned>(dip+1) };
        }

        ID3D12QueryHeap* native()
        {
            return heap.Get();
        }

        size_t size()
        {
            return m_size;
        }

        size_t counterSize()
        {
            return sizeof(uint64_t);
        }
    };

    class DX12ReadbackHeap
    {
      LinearAllocator allocator;
      ComPtr<ID3D12Resource> resource;
      unsigned fixedSize = 1;
      unsigned size = 1;

      uint8_t* data = nullptr;
      D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;

      D3D12_RANGE range{};
    public:
      DX12ReadbackHeap() : allocator(1) {}
      DX12ReadbackHeap(ID3D12Device* device, unsigned allocationSize, unsigned allocationCount)
        : allocator(allocationSize * allocationCount)
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
          heap.Type = D3D12_HEAP_TYPE_READBACK;
          heap.CreationNodeMask = 0;

          FAZE_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &dxdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource)));

          gpuAddr = resource->GetGPUVirtualAddress();
        }

        range.End = allocationSize * allocationCount;
      }

      ReadbackBlock allocate(size_t bytes)
      {
        auto dip = allocator.allocate(bytes, 512);
        F_ASSERT(dip != -1, "No space left, make bigger DX12ReadbackHeap :) %d", bytes);
        return ReadbackBlock{ static_cast<size_t>(dip),  bytes};
      }

      void reset()
      {
          allocator = LinearAllocator(size);
      }

      void map()
      {
        FAZE_CHECK_HR(resource->Map(0, &range, reinterpret_cast<void**>(&data)));
      }

      void unmap()
      {
        D3D12_RANGE range2{};
        resource->Unmap(0u, &range2);
      }

      faze::MemView<uint8_t> getView(ReadbackBlock block)
      {
        return faze::MemView<uint8_t>(data + block.offset, block.m_size);
      }

      ID3D12Resource* native()
      {
        return resource.Get();
      }
    };

    struct DX12ReadbackLambda
    {
      ReadbackBlock dataLocation;
      std::function<void(MemView<uint8_t>)> func;
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
      std::shared_ptr<DX12DependencySolver> m_solver;
    public:
      //DX12CommandBuffer() {}
      DX12CommandBuffer(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandAllocator> commandListAllocator);

      DX12CommandBuffer(DX12CommandBuffer&& other) = default;
      DX12CommandBuffer(const DX12CommandBuffer& other) = delete;

      DX12CommandBuffer& operator=(DX12CommandBuffer&& other) = default;
      DX12CommandBuffer& operator=(const DX12CommandBuffer& other) = delete;

      ID3D12GraphicsCommandList* list();
      DX12DependencySolver* solver();
      void closeList();
      void resetList();
      bool closed() const;
    };
    struct FreeableResources
    {
      vector<UploadBlock> uploadBlocks;
      vector<DynamicDescriptorBlock> descriptorBlocks;
      vector<DX12ReadbackLambda> readbacks;
      vector<QueryBracket> queries;
    };
    class DX12CommandList : public CommandBufferImpl
    {
      std::shared_ptr<DX12CommandBuffer> m_buffer;
      std::shared_ptr<DX12UploadHeap> m_constants;
      std::shared_ptr<DX12UploadHeap> m_upload;
      std::shared_ptr<DX12ReadbackHeap> m_readback;
      std::shared_ptr<DX12QueryHeap> m_queryheap;
      std::shared_ptr<DX12DynamicDescriptorHeap> m_descriptors;
      DX12CPUDescriptor m_nullBufferUAV;
      DX12CPUDescriptor m_nullBufferSRV;

      UploadLinearAllocator m_constantsAllocator;
      LinearDescriptorAllocator m_descriptorAllocator;

      std::shared_ptr<FreeableResources> m_freeResources;

      UploadBlock allocateConstants(size_t size);
      DynamicDescriptorBlock allocateDescriptors(size_t size);
      void handleBindings(DX12Device* dev, ID3D12GraphicsCommandList*, gfxpacket::ResourceBinding& binding);
      void addCommands(DX12Device* dev, ID3D12GraphicsCommandList* buffer, DX12DependencySolver* solver, backend::IntermediateList& list);
      void addDepedencyDataAndSolve(DX12DependencySolver* solver, backend::IntermediateList& list);
      void processRenderpasses(DX12Device* dev, backend::IntermediateList& list);
    public:
        DX12CommandList(
        std::shared_ptr<DX12CommandBuffer> buffer,
        std::shared_ptr<DX12UploadHeap> constants,
        std::shared_ptr<DX12UploadHeap> dynamicUpload,
        std::shared_ptr<DX12ReadbackHeap> readback,
        std::shared_ptr<DX12QueryHeap> queryheap,
        std::shared_ptr<DX12DynamicDescriptorHeap> descriptors,
        DX12CPUDescriptor nullBufferUAV,
        DX12CPUDescriptor nullBufferSRV)
        : m_buffer(buffer)
        , m_constants(constants)
        , m_upload(dynamicUpload)
        , m_readback(readback)
        , m_queryheap(queryheap)
        , m_descriptors(descriptors)
        , m_nullBufferUAV(nullBufferUAV)
        , m_nullBufferSRV(nullBufferSRV)
      {
        m_buffer->resetList();
        m_readback->reset();
        m_queryheap->reset();

        std::weak_ptr<DX12UploadHeap> consts = m_constants;
        std::weak_ptr<DX12DynamicDescriptorHeap> descriptrs = m_descriptors;
        std::weak_ptr<DX12ReadbackHeap> read = readback;

        m_freeResources = std::shared_ptr<FreeableResources>(new FreeableResources, [consts, descriptrs, read](FreeableResources* ptr)
        {
          if (auto constants = consts.lock())
          {
            for (auto&& it : ptr->uploadBlocks)
            {
              constants->release(it);
            }
          }

          if (auto descriptors = descriptrs.lock())
          {
            for (auto&& it : ptr->descriptorBlocks)
            {
              descriptors->release(it);
            }
          }

          // handle readbacks
          if (auto readback = read.lock())
          {
              if (!ptr->readbacks.empty())
              {
                  readback->map();
                  for (auto&& it : ptr->readbacks)
                  {
                      it.func(readback->getView(it.dataLocation));
                  }
                  readback->unmap();
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
      ComPtr<D3D12Swapchain> m_resource;
      DX12GraphicsSurface m_surface;
      HANDLE m_frameLatencyObj;
      int m_backbufferIndex = 0;
      bool supportsHDR = false;
      DisplayCurve curve = DisplayCurve::sRGB;
      DXGI_COLOR_SPACE_TYPE colorSpace;

      struct Desc
      {
        int width = 0;
        int height = 0;
        SwapchainDescriptor descriptor;
      } m_desc;

    public:
      DX12Swapchain()
      {}
      DX12Swapchain(ComPtr<D3D12Swapchain> resource, DX12GraphicsSurface surface, HANDLE frameLatObject)
        : m_resource(resource)
        , m_surface(surface)
        , m_frameLatencyObj(frameLatObject)
      {}

      ResourceDescriptor desc() override
      {
        return ResourceDescriptor()
          .setWidth(m_desc.width)
          .setHeight(m_desc.height)
          .setFormat(m_desc.descriptor.desc.format)
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

      void setHDR(bool hdr)
      {
        supportsHDR = hdr;
      }

      void setDisplayCurve(DisplayCurve value)
      {
        curve = value;
      }

      void setNativeColorspace(DXGI_COLOR_SPACE_TYPE value)
      {
        colorSpace = value;
      }

      DXGI_COLOR_SPACE_TYPE nativeColorspace()
      {
        return colorSpace;
      }

      bool HDRSupport() override
      {
        return supportsHDR;
      }

      DisplayCurve displayCurve() override
      {
        return curve;
      }

      void setBufferMetadata(int x, int y, SwapchainDescriptor descriptor)
      {
        m_desc.width = x;
        m_desc.height = y;
        m_desc.descriptor = descriptor;
      }

      Desc getDesc()
      {
        return m_desc;
      }

      D3D12Swapchain* native()
      {
        return m_resource.Get();
      }

      DX12GraphicsSurface& surface()
      {
        return m_surface;
      }

      HANDLE frameLatencyObj()
      {
        return m_frameLatencyObj;
      }

      void setFrameLatencyObj(HANDLE h)
      {
        m_frameLatencyObj = h;
      }

      void setSwapchain(ComPtr<D3D12Swapchain> h)
      {
        m_resource = h;
      }

      void waitForFrameLatency()
      {
        WaitForSingleObjectEx(m_frameLatencyObj, 1000, TRUE);
      }

      void setBackbufferIndex(int index)
      {
        m_backbufferIndex = index;
      }
    };

    struct DX12ResourceState
    {
      vector<D3D12_RESOURCE_STATES> flags; // subresource count
    };

    class DX12Texture : public prototypes::TextureImpl
    {
    private:
      ID3D12Resource * resource = nullptr;
      std::shared_ptr<DX12ResourceState> statePtr = nullptr;

    public:
      DX12Texture()
      {}
      DX12Texture(ID3D12Resource* resource, std::shared_ptr<DX12ResourceState> state)
        : resource(resource)
        , statePtr(state)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }

      std::shared_ptr<DX12ResourceState> state()
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
      DX12CPUDescriptor resource;
      SubresourceRange subResourceRange;
    public:
      DX12TextureView()
      {}
      DX12TextureView(DX12CPUDescriptor resource, SubresourceRange subResourceRange)
        : resource(resource)
        , subResourceRange(subResourceRange)
      {}
      DX12CPUDescriptor native()
      {
        return resource;
      }

      SubresourceRange range()
      {
        return subResourceRange;
      }

      backend::RawView view() override
      {
        backend::RawView view{};
        view.view = resource.cpu.ptr;
        return view;
      }
    };

    class DX12Buffer : public prototypes::BufferImpl
    {
    private:
      ID3D12Resource * resource;
      std::shared_ptr<DX12ResourceState> statePtr = nullptr;

    public:
      DX12Buffer()
      {}
      DX12Buffer(ID3D12Resource* resource, std::shared_ptr<DX12ResourceState> state)
        : resource(resource)
        , statePtr(state)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }
      std::shared_ptr<DX12ResourceState> state()
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
      DX12CPUDescriptor resource;

    public:
      DX12BufferView()
      {}
      DX12BufferView(DX12CPUDescriptor resource)
        : resource(resource)
      {}
      DX12CPUDescriptor native()
      {
        return resource;
      }

      backend::RawView view() override
      {
        backend::RawView view{};
        view.view = resource.cpu.ptr;
        return view;
      }
    };

    class DX12DynamicBufferView : public prototypes::DynamicBufferViewImpl
    {
    private:
      UploadBlock block;
      DX12CPUDescriptor resource;
      DXGI_FORMAT format;
      int m_rowPitch = -1;
      unsigned m_stride = 1;
      friend class DX12Device;
    public:
      DX12DynamicBufferView()
      {
      }
      DX12DynamicBufferView(UploadBlock block, size_t rowPitch)
        : block(block)
        , format(DXGI_FORMAT_UNKNOWN)
        , m_rowPitch(static_cast<int>(rowPitch))
      {
      }

      DX12DynamicBufferView(UploadBlock block, DX12CPUDescriptor resource, DXGI_FORMAT format, unsigned stride)
        : block(block)
        , resource(resource)
        , format(format)
        , m_stride(stride)
      {
      }

      DX12CPUDescriptor native()
      {
        return resource;
      }

      D3D12_INDEX_BUFFER_VIEW indexBufferView()
      {
        D3D12_INDEX_BUFFER_VIEW view{};
        view.BufferLocation = block.gpuVirtualAddress();
        view.Format = format;
        view.SizeInBytes = static_cast<unsigned>(block.size());
        return view;
      }

      backend::RawView view() override
      {
        backend::RawView view{};
        view.view = resource.cpu.ptr;
        return view;
      }

      int rowPitch() override
      {
        return m_rowPitch;
      }

      uint64_t offset() override
      {
        return static_cast<uint64_t>(block.block.offset) / m_stride;
      }

      uint64_t size() override
      {
        return static_cast<uint64_t>(block.block.size) / m_stride;
      }
    };

    class DX12Heap : public prototypes::HeapImpl
    {
    private:
      ComPtr<ID3D12Heap> heap;

    public:
      DX12Heap()
      {}
      DX12Heap(ComPtr<ID3D12Heap> heap)
        : heap(heap)
      {}
      ID3D12Heap* native()
      {
        return heap.Get();
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
      D3D12_PRIMITIVE_TOPOLOGY primitive;
      DX12Pipeline()
        : pipeline(nullptr)
        , root(nullptr)
        , primitive(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
      {
      }
      DX12Pipeline(ComPtr<ID3D12PipelineState> pipeline, ComPtr<ID3D12RootSignature> root, D3D12_PRIMITIVE_TOPOLOGY primitive)
        : pipeline(pipeline)
        , root(root)
        , primitive(primitive)
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

      Rabbitpool2<DX12ReadbackHeap> m_readbackPool; // make this a new kind of pool, which keeps alive recently used.
      Rabbitpool2<DX12QueryHeap> m_queryHeapPool;
      Rabbitpool2<DX12CommandBuffer> m_copyListPool;
      Rabbitpool2<DX12CommandBuffer> m_computeListPool;
      Rabbitpool2<DX12CommandBuffer> m_graphicsListPool;
      Rabbitpool2<DX12Fence> m_fencePool;

      std::shared_ptr<DX12UploadHeap> m_constantsUpload;
      std::shared_ptr<DX12UploadHeap> m_dynamicUpload;

      std::shared_ptr<DX12DynamicDescriptorHeap> m_dynamicGpuDescriptors;

      std::shared_ptr<SequenceTracker> m_seqTracker;

      DX12CPUDescriptor m_nullBufferUAV;
      DX12CPUDescriptor m_nullBufferSRV;

      struct Garbage
      {
        vector<UploadBlock> dynamicBuffers;
        vector<DX12CPUDescriptor> genericDescriptors;
        vector<DX12CPUDescriptor> rtvsDescriptors;
        vector<DX12CPUDescriptor> dsvsDescriptors;
        vector<ID3D12Resource*> resources;
        vector<ComPtr<ID3D12PipelineState>> pipelines;
        vector<ComPtr<ID3D12RootSignature>> roots;
      };

      std::shared_ptr<Garbage> m_trash;
      deque<std::pair<SeqNum, Garbage>> m_collectableTrash;

      friend class DX12CommandList;
    public:
      DX12Device(GpuInfo info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory4> factory, FileSystem& fs);
      ~DX12Device();

      D3D12_RESOURCE_DESC fillPlacedBufferInfo(ResourceDescriptor descriptor);
      D3D12_RESOURCE_DESC fillPlacedTextureInfo(ResourceDescriptor descriptor);

      void updatePipeline(GraphicsPipeline& pipeline, gfxpacket::Subpass& subpass);
      void updatePipeline(ComputePipeline& pipeline);

      // impl
      std::shared_ptr<prototypes::PipelineImpl> createPipeline() override;

      std::shared_ptr<prototypes::SwapchainImpl> createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor) override;
      void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, SwapchainDescriptor descriptor) override;
      void ensureSwapchainColorspace(std::shared_ptr<DX12Swapchain> sc, SwapchainDescriptor& descriptor);
      void destroySwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      vector<std::shared_ptr<prototypes::TextureImpl>> getSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;

      void collectTrash() override;
      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      std::shared_ptr<prototypes::RenderpassImpl> createRenderpass() override;

      GpuHeap createHeap(HeapDescriptor desc) override;
      void destroyHeap(GpuHeap heap) override;

      std::shared_ptr<prototypes::BufferImpl> createBuffer(HeapAllocation allocation, ResourceDescriptor& desc) override;
      std::shared_ptr<prototypes::BufferViewImpl> createBufferView(std::shared_ptr<prototypes::BufferImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      std::shared_ptr<prototypes::TextureImpl> createTexture(HeapAllocation allocation, ResourceDescriptor& desc) override;
      std::shared_ptr<prototypes::TextureViewImpl> createTextureView(std::shared_ptr<prototypes::TextureImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;

      std::shared_ptr<prototypes::DynamicBufferViewImpl> dynamic(MemView<uint8_t> bytes, FormatType format) override;
      std::shared_ptr<prototypes::DynamicBufferViewImpl> dynamic(MemView<uint8_t> bytes, unsigned stride) override;
      std::shared_ptr<prototypes::DynamicBufferViewImpl> dynamicImage(MemView<uint8_t> bytes, unsigned rowPitch) override;

      // empty, these don't do anything.
      void destroyBuffer(std::shared_ptr<prototypes::BufferImpl> buffer) override;
      void destroyBufferView(std::shared_ptr<prototypes::BufferViewImpl> buffer) override;
      void destroyTexture(std::shared_ptr<prototypes::TextureImpl> buffer) override;
      void destroyTextureView(std::shared_ptr<prototypes::TextureViewImpl> buffer) override;

      // commandlist things and gpu-cpu/gpu-gpu synchronization primitives
      DX12QueryHeap createQueryHeap(unsigned counters);
      DX12ReadbackHeap createReadback(unsigned pages, unsigned pageSize);
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