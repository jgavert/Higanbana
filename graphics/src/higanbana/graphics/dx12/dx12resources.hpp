#pragma once
#include <higanbana/core/platform/definitions.hpp>
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/desc/timing.hpp"
#include "higanbana/graphics/dx12/dx12.hpp"
#include "higanbana/graphics/shaders/ShaderStorage.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/allocators.hpp"
#include "higanbana/graphics/definitions.hpp"
#include <higanbana/core/system/MemoryPools.hpp>
#include <higanbana/core/system/MovePtr.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <higanbana/core/system/heap_allocator.hpp>
#include <DXGIDebug.h>

#include <algorithm>
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
      RangeBlock block;

      DX12GPUDescriptor offset(int index) const
      {
        HIGAN_ASSERT(index < block.size && index >= 0, "Invalid index %d", index);
        DX12GPUDescriptor desc{};
        desc.cpu.ptr = baseCpuHandle.ptr + static_cast<size_t>(index + block.offset) * increment;
        desc.gpu.ptr = baseGpuHandle.ptr + static_cast<size_t>(index + block.offset) * increment;
        return desc;
      }

      operator bool() const
      {
        return block;
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
          return DynamicDescriptorBlock{ 0, 0, 0, RangeBlock{} };

        DynamicDescriptorBlock b = block;
        b.block.offset += offset;
        b.block.size = bytes;
        return b;
      }
    };

    class DX12ShaderArguments
    {
      public:
      DynamicDescriptorBlock descriptorTable;
      DX12ShaderArguments(){}

      DX12ShaderArguments(DynamicDescriptorBlock block)
        : descriptorTable(block)
        {}

    };

    class DX12DynamicDescriptorHeap
    {
      HeapAllocator allocator;
      ComPtr<ID3D12DescriptorHeap> heap;
      D3D12_DESCRIPTOR_HEAP_TYPE type;
      DynamicDescriptorBlock baseRange;
      int m_size = -1;
    public:
      DX12DynamicDescriptorHeap()
        : allocator(0, 0)
      {}
      DX12DynamicDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, int descriptorCount)
        : allocator(descriptorCount, 1)
        , type(type)
        , m_size(descriptorCount)
      {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = type;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;
        desc.NumDescriptors = static_cast<UINT>(m_size);

        HIGANBANA_CHECK_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

        baseRange.baseCpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        baseRange.baseGpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
        baseRange.increment = device->GetDescriptorHandleIncrementSize(type);
      }

      DynamicDescriptorBlock allocate(int value)
      {
        auto offset = allocator.allocate(value);
        HIGAN_ASSERT(offset && offset.value().size > 0, "No descriptors left, make bigger Staging :) %d type: %d", max_size(), static_cast<int>(type));
        DynamicDescriptorBlock desc = baseRange;
        desc.block = offset.value();
        return desc;
      }

      void release(DynamicDescriptorBlock range)
      {
        HIGAN_ASSERT(range.block.offset != -1, "halp");
        allocator.free(range.block);
      }

      ID3D12DescriptorHeap* native()
      {
        return heap.Get();
      }

      size_t size() const noexcept
      {
        return allocator.size();
      }
      size_t max_size() const noexcept
      {
        return allocator.max_size();
      }
      size_t size_allocated() const noexcept
      {
        return allocator.size_allocated();
      }
    };

    class StagingDescriptorHeap
    {
      HeapAllocator allocator;
      ComPtr<ID3D12DescriptorHeap> heap;
      D3D12_DESCRIPTOR_HEAP_TYPE type;
      D3D12_CPU_DESCRIPTOR_HANDLE start;
      UINT increment;

      int m_size = -1;
    public:
      StagingDescriptorHeap() {}
      StagingDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, int count)
        : allocator(count, 1)
        , type(type)
        , m_size(count)
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
        HIGAN_ASSERT(dip && dip.value().size > 0, "No descriptors left, make bigger Staging :) %d type: %d", max_size(), static_cast<int>(type));
        DX12CPUDescriptor desc{};
        desc.block = dip.value();
        desc.type = type;
        desc.cpu.ptr = start.ptr + increment * dip.value().offset;
        return desc;
      }

      void release(DX12CPUDescriptor desc)
      {
        allocator.free(desc.block);
      }

      size_t size() const noexcept
      {
        return allocator.size();
      }
      size_t max_size() const noexcept
      {
        return allocator.max_size();
      }
    };

    struct UploadBlock
    {
      uint8_t* m_data;
      D3D12_GPU_VIRTUAL_ADDRESS m_resourceAddress;
      ID3D12Resource* m_resPtr;
      RangeBlock block;

      ID3D12Resource* native()
      {
        return m_resPtr;
      }

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
          return UploadBlock{ nullptr, 0, nullptr, RangeBlock{} };

        UploadBlock b = block;
        b.block.offset += offset;
        HIGAN_ASSERT(b.block.offset % alignment == 0, "hupsies");
        b.block.size = roundUpMultipleInt(bytes, alignment);
        HIGAN_ASSERT(b.block.size >= bytes, "hupsies v2");
        return b;
      }
    };

    // TODO: protect with mutex
    class DX12UploadHeap
    {
      HeapAllocator allocator;
      ComPtr<ID3D12Resource> resource;

      uint8_t* data = nullptr;
      D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;
    public:
      DX12UploadHeap() : allocator(1, 1) {}
      DX12UploadHeap(ID3D12Device* device, unsigned memoryAmount)
        : allocator(memoryAmount, 16)
      {
        {
          D3D12_RESOURCE_DESC dxdesc{};

          dxdesc.Width = memoryAmount;
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
        range.End = memoryAmount;

        HIGANBANA_CHECK_HR(resource->Map(0, &range, reinterpret_cast<void**>(&data)));
      }

      UploadBlock allocate(size_t bytes, size_t alignment)
      {
        auto dip = allocator.allocate(bytes, alignment);
        HIGAN_ASSERT(dip.has_value(), "No space left, make bigger DX12UploadHeap :) %d", max_size());
        HIGAN_ASSERT(dip.value().offset % alignment == 0, "oh no");
        return UploadBlock{ data, gpuAddr, resource.Get(), dip.value()};
      }

      void release(UploadBlock desc)
      {
        allocator.free(desc.block);
      }

      ID3D12Resource* native()
      {
        return resource.Get();
      }
      
      size_t size() const noexcept
      {
        return allocator.size();
      }
      size_t max_size() const noexcept
      {
        return allocator.max_size();
      }
      size_t size_allocated() const noexcept
      {
        return allocator.size_allocated();
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
      DX12QueryHeap(ID3D12Device* device, ID3D12CommandQueue* queue, D3D12_QUERY_HEAP_TYPE type, unsigned counterCount)
        : allocator(counterCount)
        , m_size(counterCount)
      {
        static int queryHeapCount = 0;

        D3D12_QUERY_HEAP_DESC desc{};
        desc.Type = type;
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
        HIGAN_ASSERT(dip != -1, "Queryheap out of queries.");
        return DX12Query{ static_cast<unsigned>(dip),  static_cast<unsigned>(dip + 1) };
      }

      ID3D12QueryHeap* native()
      {
        return heap.Get();
      }

      size_t size() const noexcept
      {
        return allocator.size();
      }

      size_t max_size() const noexcept
      {
        return allocator.max_size();
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
      unsigned m_size = 1;

      uint8_t* data = nullptr;
      D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;

      D3D12_RANGE range{};
    public:
      DX12ReadbackHeap() : allocator(1) {}
      DX12ReadbackHeap(ID3D12Device* device, unsigned allocationSize, unsigned allocationCount)
        : allocator(allocationSize * allocationCount)
        , fixedSize(allocationSize)
        , m_size(allocationSize*allocationCount)
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
        HIGAN_ASSERT(dip != -1, "No space left, make bigger DX12ReadbackHeap :) %d", bytes);
        return ReadbackBlock{ static_cast<size_t>(dip),  bytes };
      }

      void reset()
      {
        allocator = LinearAllocator(m_size);
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

      size_t size() const noexcept
      {
        return allocator.size();
      }

      size_t max_size() const noexcept
      {
        return allocator.max_size();
      }
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
        HIGAN_ASSERT(WAIT_OBJECT_0 == result, "Fence wait failed.");
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
      //vector<DX12Readback> readbacks;
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
      bool m_outOfDate = false;

      struct Desc
      {
        int2 res;
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
          .setWidth(m_desc.res.x)
          .setHeight(m_desc.res.y)
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

      void setOutOfDate(bool value)
      {
        m_outOfDate = value;
      }

      bool outOfDate() override
      {
        return m_outOfDate;
      }

      void setBufferMetadata(int x, int y, SwapchainDescriptor descriptor)
      {
        m_desc.res = int2(x, y);
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
      ResourceDescriptor descriptor;

    public:
      DX12Buffer()
      {}
      DX12Buffer(ID3D12Resource* resource, ResourceDescriptor descriptor)
        : resource(resource)
        , descriptor(descriptor)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }

      ResourceDescriptor& desc()
      {
        return descriptor;
      }
    };

    class DX12Readback
    {
      ID3D12Resource* buffer;
      size_t m_offset;
      size_t m_size;
      uint8_t* m_data;
      bool m_mapped = false;
    public:
      DX12Readback()
        : buffer(nullptr)
        , m_offset(0)
        , m_size(0)
        , m_data(nullptr)
        , m_mapped(false)
      {}
      DX12Readback(ID3D12Resource* resource, size_t offset, size_t size)
        : buffer(resource)
        , m_offset(offset)
        , m_size(size)
        , m_data(nullptr)
        , m_mapped(false)
      {}
      ID3D12Resource* native()
      {
        return buffer;
      }
      size_t offset() const
      {
        return m_offset;
      }
      size_t size() const
      {
        return m_size;
      }
      uint8_t* map()
      {
        HIGAN_ASSERT(!m_mapped, "Resource is already mapped.");
        D3D12_RANGE range;
        range.Begin = m_offset;
        range.End = m_offset+m_size;
        HIGANBANA_CHECK_HR(buffer->Map(0, &range, reinterpret_cast<void**>(&m_data)));
        m_mapped = true;
        return m_data;
      }
      void unmap()
      {
        HIGAN_ASSERT(m_mapped, "Resource wasn't mapped.");
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = 0;
        buffer->Unmap(0, &range);
        m_mapped = false;
        m_data = nullptr;
      }
    };

    class DX12BufferView 
    {
    private:
      DX12CPUDescriptor resource;
      ID3D12Resource* refResource;

    public:
      DX12BufferView()
      {}
      DX12BufferView(DX12CPUDescriptor resource)
        : resource(resource)
        , refResource(nullptr)
      {}
      DX12BufferView(DX12CPUDescriptor resource, ID3D12Resource* refRes)
        : resource(resource)
        , refResource(refRes)
      {}
      DX12CPUDescriptor native()
      {
        return resource;
      }
      ID3D12Resource* ref()
      {
        return refResource;
      }
    };

    class DX12DynamicBufferView 
    {
    private:
      UploadBlock block = {0,0};
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
      ID3D12Resource* nativePtr()
      {
        return block.native();
      }
      int rowPitch()
      {
        return m_rowPitch;
      }
      uint64_t offset()
      {
        return static_cast<uint64_t>(block.block.offset);
      }
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

      vector<std::pair<WatchFile, ShaderType>> m_watchedShaders;
      WatchFile cs;
      bool needsUpdating() noexcept
      {
        return std::any_of(m_watchedShaders.begin(), m_watchedShaders.end(), [](std::pair<WatchFile, ShaderType>& shader){ return shader.first.updated();});
      }

      void updated()
      {
        std::for_each(m_watchedShaders.begin(), m_watchedShaders.end(), [](std::pair<WatchFile, ShaderType>& shader){ shader.first.react();});
      }
    };
    struct DX12Resources
    {
      HandleVector<DX12Texture> tex;
      HandleVector<DX12Buffer> buf;
      HandleVector<DX12Readback> rbbuf;
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
      HandleVector<DX12ShaderArguments> shaArgs;
    };
  }
}

#endif