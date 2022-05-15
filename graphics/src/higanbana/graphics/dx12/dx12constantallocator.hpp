#pragma once
#include <higanbana/core/platform/definitions.hpp>
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/dx12/dx12.hpp"
#include "higanbana/graphics/common/allocators.hpp"
#include "higanbana/graphics/definitions.hpp"
#include <higanbana/core/system/heap_allocator.hpp>

#include <mutex>

namespace higanbana
{
  namespace backend
  {
    struct DX12ConstantAllocatorBlock 
    {
      uint8_t* m_data;
      D3D12_GPU_VIRTUAL_ADDRESS m_resourceAddress; // gpu
      ID3D12Resource* m_resSrcPtr;
      ID3D12Resource* m_resGpuPtr;
      RangeBlock block;

      ID3D12Resource* stagingRes()
      {
        return m_resSrcPtr;
      }
      ID3D12Resource* gpuRes()
      {
        return m_resGpuPtr;
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

    class DX12LinearConstantsAllocator : public LinearConstantsAllocator
    {
      LinearAllocator allocator;
      DX12ConstantAllocatorBlock m_block;

    public:
      DX12LinearConstantsAllocator() {}
      DX12LinearConstantsAllocator(DX12ConstantAllocatorBlock block)
        : allocator(block.size())
        , m_block(block)
      {
      }

      ConstantsBlock allocate(size_t bytes) override // alignment
      {
        auto offset = allocator.allocate(bytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);//, alignment);
        if (offset < 0)
          return ConstantsBlock{ 0ull, nullptr};

        DX12ConstantAllocatorBlock b = m_block;
        b.block.offset += offset;
        HIGAN_ASSERT(b.block.offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "hupsies");
        b.block.size = roundUpMultiplePowerOf2(bytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        HIGAN_ASSERT(b.block.size >= bytes, "hupsies v2");
        return ConstantsBlock{b.gpuVirtualAddress(), b.data()};
      }

      DX12ConstantAllocatorBlock block()
      {
        return m_block;
      }
    };

    class DX12ConstantsAllocator : public ConstantsAllocator
    {
    public:
      enum class Type {
        OnlyCPU,
        CPUAndGPU
      };
    private:
      HeapAllocator allocator;
      ComPtr<ID3D12Resource> resourceCPU; // where constants are copied on cpu
      ComPtr<ID3D12Resource> resourceGPU; // where gpu accesses the constants

      uint8_t* data = nullptr;
      D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;
      Type m_type;
      std::mutex m_allocatorLock;
    public:
      DX12ConstantsAllocator() : allocator(1, 1), m_type(Type::OnlyCPU) {}
      DX12ConstantsAllocator(ID3D12Device* device, unsigned memoryAmount, Type type)
        : allocator(memoryAmount, 16)
        , m_type(type)
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

          HIGANBANA_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &dxdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resourceCPU)));
          gpuAddr = resourceCPU->GetGPUVirtualAddress();
          if (type == Type::CPUAndGPU) {
            heap.Type = D3D12_HEAP_TYPE_DEFAULT;

            HIGANBANA_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &dxdesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resourceGPU)));

            gpuAddr = resourceGPU->GetGPUVirtualAddress();
          }
        }

        D3D12_RANGE range{};
        range.End = memoryAmount;

        HIGANBANA_CHECK_HR(resourceCPU->Map(0, &range, reinterpret_cast<void**>(&data)));
      }

      LinearConstantsAllocator* allocate(size_t bytes) override //, size_t alignment)
      {
        std::lock_guard guard(m_allocatorLock);
        auto dip = allocator.allocate(bytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);//, alignment);
        HIGAN_ASSERT(dip.has_value(), "No space left, make bigger DX12UploadHeap :) %d", allocator.max_size());
        HIGAN_ASSERT(dip.value().offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "oh no");
        return new DX12LinearConstantsAllocator(DX12ConstantAllocatorBlock{ data, gpuAddr, resourceCPU.Get(), resourceGPU.Get(), dip.value()});
        //return UploadBlockGPU{ data, gpuAddr, resourceCPU.Get(), resourceGPU.Get(), dip.value()};
      }

      void free(LinearConstantsAllocator* ptr) override
      {
        DX12LinearConstantsAllocator* nptr = static_cast<DX12LinearConstantsAllocator*>(ptr);
        std::lock_guard guard(m_allocatorLock);
        allocator.free(nptr->block().block);
        delete nptr;
      }

/*
      void releaseVector(const vector<UploadBlockGPU>& descs)
      {
        std::lock_guard guard(m_allocatorLock);
        for (auto&& desc : descs)
          allocator.free(desc.block);
      }
*/
      ID3D12Resource* nativeCPU()
      {
        return resourceCPU.Get();
      }
      ID3D12Resource* nativeGPU()
      {
        return resourceGPU.Get();
      }
      
      size_t size() override
      {
        std::lock_guard guard(m_allocatorLock);
        return allocator.findLargestAllocation();
      }
      size_t max_size() override
      {
        std::lock_guard guard(m_allocatorLock);
        return allocator.max_size();
      }
      size_t size_allocated() override
      {
        std::lock_guard guard(m_allocatorLock);
        return allocator.size_allocated();
      }
    };
  }
}
#endif