#pragma once
#include <higanbana/core/platform/definitions.hpp>
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/dx12/dx12.hpp"
#include "higanbana/graphics/shaders/ShaderStorage.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/allocators.hpp"
#include "higanbana/graphics/definitions.hpp"
#include <higanbana/core/system/MemoryPools.hpp>
#include <higanbana/core/system/MovePtr.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <DXGIDebug.h>

#include <memory>

namespace higanbana
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

        HIGANBANA_CHECK_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

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

        HIGANBANA_CHECK_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));

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

          HIGANBANA_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &dxdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));

          gpuAddr = resource->GetGPUVirtualAddress();
        }

        D3D12_RANGE range{};
        range.End = allocationSize * allocationCount;

        HIGANBANA_CHECK_HR(resource->Map(0, &range, reinterpret_cast<void**>(&data)));
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

        HIGANBANA_CHECK_HR(device->CreateQueryHeap(&desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));
        auto name = s2ws("QueryHeap" + std::to_string(queryHeapCount));
        heap->SetName(name.c_str());

        HIGANBANA_CHECK_HR(queue->GetTimestampFrequency(&m_gpuTicksPerSecond));
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
        return DX12Query{ static_cast<unsigned>(dip),  static_cast<unsigned>(dip + 1) };
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

          HIGANBANA_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &dxdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource)));

          gpuAddr = resource->GetGPUVirtualAddress();
        }

        range.End = allocationSize * allocationCount;
      }

      ReadbackBlock allocate(size_t bytes)
      {
        auto dip = allocator.allocate(bytes, 512);
        F_ASSERT(dip != -1, "No space left, make bigger DX12ReadbackHeap :) %d", bytes);
        return ReadbackBlock{ static_cast<size_t>(dip),  bytes };
      }

      void reset()
      {
        allocator = LinearAllocator(size);
      }

      void map()
      {
        HIGANBANA_CHECK_HR(resource->Map(0, &range, reinterpret_cast<void**>(&data)));
      }

      void unmap()
      {
        D3D12_RANGE range2{};
        resource->Unmap(0u, &range2);
      }

      higanbana::MemView<uint8_t> getView(ReadbackBlock block)
      {
        return higanbana::MemView<uint8_t>(data + block.offset, block.m_size);
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

    class DX12Fence : public FenceImpl
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

    class DX12Semaphore : public SemaphoreImpl
    {
    public:
      ComPtr<ID3D12Fence> fence = nullptr;
      std::shared_ptr<uint64_t> value = nullptr;

      DX12Semaphore()
      {}

      DX12Semaphore(ComPtr<ID3D12Fence> fence)
        : fence(fence)
        , value(std::make_shared<uint64_t>(0))
      {
      }

      uint64_t start()
      {
        return ++(*value);
      }
    };

    class DX12CommandBuffer
    {
      ComPtr<D3D12GraphicsCommandList> commandList;
      ComPtr<ID3D12CommandAllocator> commandListAllocator;
      bool closedList = false;
      bool dmaList;
    public:
      //DX12CommandBuffer() {}
      DX12CommandBuffer(ComPtr<D3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandAllocator> commandListAllocator, bool isDma);

      DX12CommandBuffer(DX12CommandBuffer&& other) = default;
      DX12CommandBuffer(const DX12CommandBuffer& other) = delete;

      DX12CommandBuffer& operator=(DX12CommandBuffer&& other) = default;
      DX12CommandBuffer& operator=(const DX12CommandBuffer& other) = delete;

      D3D12GraphicsCommandList* list();
      void closeList();
      void resetList();
      bool closed() const;
      bool dma() const;
    };

    struct DX12OldPipeline
    {
      ComPtr<ID3D12PipelineState> pipeline;
      ComPtr<ID3D12RootSignature> root;
    };

    struct FreeableResources
    {
      vector<UploadBlock> uploadBlocks;
      vector<DynamicDescriptorBlock> descriptorBlocks;
      vector<DX12ReadbackLambda> readbacks;
      vector<QueryBracket> queries;
      vector<DX12OldPipeline> pipelines;
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

      bool checkFrameLatency()
      {
          auto result = WaitForSingleObjectEx(m_frameLatencyObj, 0, FALSE);

        return result == WAIT_OBJECT_0;
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
      bool commonStateOptimisation;
      vector<D3D12_RESOURCE_STATES> flags; // subresource count
    };

    class DX12Texture
    {
    private:
      ID3D12Resource* resource = nullptr;
      ResourceDescriptor descriptor;
      int maxMipSize = 1;

    public:
      DX12Texture()
      {}
      DX12Texture(ID3D12Resource* resource, ResourceDescriptor descriptor, int maxMipSize)
        : resource(resource)
        , descriptor(descriptor)
        , maxMipSize(maxMipSize)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }

      const ResourceDescriptor& desc() const
      {
        return descriptor;
      }

      int mipSize() const
      {
        return maxMipSize;
      }

      void resetState() // wtf is this?? kind of understand but still.
      {
        //statePtr = nullptr;
      }
    };

    class DX12TextureView 
    {
    private:
      DX12CPUDescriptor resource;
      DXGI_FORMAT m_view;
    public:
      DX12TextureView()
      {}
      DX12TextureView(DX12CPUDescriptor resource, DXGI_FORMAT view)
        : resource(resource)
        , m_view(view)
      {}
      DX12CPUDescriptor native()
      {
        return resource;
      }

      DXGI_FORMAT viewFormat() const
      {
        return m_view;
      }
    };

    class DX12Buffer 
    {
    private:
      ID3D12Resource* resource;
      std::shared_ptr<DX12ResourceState> statePtr;

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

      void resetState()
      {
        statePtr = nullptr;
      }
    };

    class DX12BufferView 
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
    };

    class DX12DynamicBufferView 
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

      bool hasDescriptor() const
      {
        return m_rowPitch == -1;
      }

      D3D12_INDEX_BUFFER_VIEW indexBufferView()
      {
        D3D12_INDEX_BUFFER_VIEW view{};
        view.BufferLocation = block.gpuVirtualAddress();
        view.Format = format;
        view.SizeInBytes = static_cast<unsigned>(block.size());
        return view;
      }

/*
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
      */
    };

    class DX12Heap
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

    class DX12Renderpass 
    {
    private:
      int _unused = -1;
    public:
      DX12Renderpass() {}
    };

    class DX12Pipeline
    {
    public:
      ComPtr<ID3D12PipelineState> pipeline;
      ComPtr<ID3D12RootSignature> root;
      D3D12_PRIMITIVE_TOPOLOGY primitive;
      bool m_hasPipeline;

      DX12Pipeline()
        : pipeline(nullptr)
        , root(nullptr)
        , primitive(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
        , m_hasPipeline(false)
      {
      }

      DX12Pipeline(ComPtr<ID3D12PipelineState> pipeline, ComPtr<ID3D12RootSignature> root, D3D12_PRIMITIVE_TOPOLOGY primitive)
        : pipeline(pipeline)
        , root(root)
        , primitive(primitive)
        , m_hasPipeline(true)
      {
      }

      GraphicsPipelineDescriptor m_gfxDesc;
      ComputePipelineDescriptor m_computeDesc;

      WatchFile vs;
      WatchFile ds;
      WatchFile hs;
      WatchFile gs;
      WatchFile ps;

      WatchFile cs;
      bool needsUpdating()
      {
        return vs.updated() || ps.updated() || ds.updated() || hs.updated() || gs.updated();
      }

      void updated()
      {
        vs.react();
        ds.react();
        hs.react();
        gs.react();
        ps.react();
      }
    };
    struct DX12Resources
    {
      HandleVector<DX12Texture> tex;
      HandleVector<DX12Buffer> buf;
      HandleVector<DX12BufferView> bufSRV;
      HandleVector<DX12BufferView> bufUAV;
      HandleVector<DX12BufferView> bufIBV;
      HandleVector<DX12TextureView> texSRV;
      HandleVector<DX12TextureView> texUAV;
      HandleVector<DX12TextureView> texDSV;
      HandleVector<DX12TextureView> texRTV;
      HandleVector<DX12DynamicBufferView> dynSRV;
      HandleVector<DX12Pipeline> pipelines;
      HandleVector<DX12Heap> heaps;
    };
  }
}

#endif