#include "higanbana/graphics/common/heap_manager.hpp"
#include "higanbana/graphics/definitions.hpp"
#include "higanbana/graphics/common/heap_descriptor.hpp"
#include "higanbana/graphics/common/helpers/memory_requirements.hpp"
#include "higanbana/graphics/common/helpers/heap_allocation.hpp"
//#include "higanbana/graphics/common/graphicssurface.hpp"
///#include "higanbana/graphics/common/implementation.hpp"
//#include "higanbana/graphics/common/semaphore.hpp"
//#include "higanbana/graphics/common/cpuimage.hpp"
//#include "higanbana/graphics/common/heap_descriptor.hpp"

#include <higanbana/core/math/utils.hpp>

namespace higanbana
{
  namespace backend
  {
    GpuHeap::GpuHeap(ResourceHandle handle, HeapDescriptor desc)
      : handle(handle)
      , desc(std::make_shared<HeapDescriptor>(desc))
    {
    }

    HeapAllocation HeapManager::allocate(MemoryRequirements requirements, std::function<GpuHeap(higanbana::HeapDescriptor)> heapAllocator)
    {
      GpuHeapAllocation alloc{};
      alloc.alignment = static_cast<int>(requirements.alignment);
      alloc.heapType = requirements.heapType;
      auto createHeapBlock = [&](uint64_t index, MemoryRequirements& requirements)
      {
        auto minSize = std::min(m_minimumHeapSize, static_cast<int64_t>(128 * 32 * requirements.alignment));
        auto sizeToCreate = roundUpMultiplePowerOf2(minSize, requirements.alignment);
        if (requirements.bytes > sizeToCreate)
        {
          sizeToCreate = roundUpMultiplePowerOf2(requirements.bytes, requirements.alignment);
        }
        std::string name;
        name += std::to_string(index);
        name += "Heap";
        name += std::to_string(requirements.heapType);
        name += "a";
        name += std::to_string(requirements.alignment);

        m_totalMemory += sizeToCreate;

        HeapDescriptor desc = HeapDescriptor()
          .setSizeInBytes(sizeToCreate)
          .setHeapType(HeapType::Custom)
          .setHeapTypeSpecific(requirements.heapType)
          .setHeapAlignment(requirements.alignment)
          .setName(name);
        GFX_LOG("Created heap \"%s\" size %.2fMB (%zu). Total memory in heaps %.2fMB\n", name.c_str(), float(sizeToCreate) / 1024.f / 1024.f, sizeToCreate, float(m_totalMemory) / 1024.f / 1024.f);
        return HeapBlock{ index, HeapAllocator(sizeToCreate, requirements.alignment), heapAllocator(desc) };
      };

      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.type == requirements.heapType;
      });
      if (vectorPtr != m_heaps.end()) // found alignment
      {
        std::optional<RangeBlock> block;
        for (auto& heap : vectorPtr->heaps)
        {
          if (heap.allocator.size() >= requirements.bytes) // this will find falsepositives if memory is fragmented.
          {
            block = heap.allocator.allocate(requirements.bytes, requirements.alignment); // should be cheap anyway
            if (block)
            {
              alloc.block = block.value();
              alloc.index = heap.index;
              m_memoryAllocated += alloc.block.size;
              return HeapAllocation{ alloc, heap.heap };
            }
          }
        }
        if (!block.has_value())
        {
          // create correct sized heap and allocate from it.
          auto newHeap = createHeapBlock(m_heapIndex++, requirements);
          auto newBlock = newHeap.allocator.allocate(requirements.bytes, requirements.alignment);
          HIGAN_ASSERT(newBlock, "block wasnt successful");
          alloc.block = newBlock.value();
          m_memoryAllocated += alloc.block.size;
          alloc.index = newHeap.index;
          auto heap = newHeap.heap;
          vectorPtr->heaps.emplace_back(std::move(newHeap));
          return HeapAllocation{ alloc, heap };
        }
      }
      HeapVector vec{ requirements.heapType, vector<HeapBlock>() };
      auto newHeap = createHeapBlock(m_heapIndex++, requirements);
      alloc.block = newHeap.allocator.allocate(requirements.bytes, requirements.alignment).value();
      m_memoryAllocated += alloc.block.size;
      alloc.index = newHeap.index;
      auto heap = newHeap.heap;
      vec.heaps.emplace_back(std::move(newHeap));
      m_heaps.emplace_back(std::move(vec));
      return HeapAllocation{ alloc, heap };
    }

    void HeapManager::release(GpuHeapAllocation object)
    {
      HIGAN_ASSERT(object.valid(), "invalid object was released");
      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.type == object.heapType;
      });
      if (vectorPtr != m_heaps.end())
      {
        auto heapPtr = std::find_if(vectorPtr->heaps.begin(), vectorPtr->heaps.end(), [&](HeapBlock& vec)
        {
          return vec.index == object.index;
        });
        m_memoryAllocated -= object.block.size;
        heapPtr->allocator.free(object.block);
      }
    }
    uint64_t HeapManager::memoryInUse()
    {
      return m_memoryAllocated;
    }
    
    uint64_t HeapManager::totalMemory()
    {
      return m_totalMemory;
    }

    vector<GpuHeap> HeapManager::emptyHeaps()
    {
      vector<GpuHeap> emptyHeaps;
      for (auto&& it : m_heaps)
      {
        for (auto iter = it.heaps.begin(); iter != it.heaps.end(); ++iter)
        {
          if (iter->allocator.size_allocated() == 0)
          {
            emptyHeaps.emplace_back(iter->heap);
            m_totalMemory -= iter->allocator.size();
            GFX_LOG("Destroyed heap \"%dHeap%zd\" size %zu. Total memory in heaps %.2fMB\n", iter->index, it.type, iter->allocator.size(), float(m_totalMemory) / 1024.f / 1024.f);
          }
        }
        for (;;)
        {
          auto removables = std::find_if(it.heaps.begin(), it.heaps.end(), [&](HeapBlock& vec)
          {
            return vec.allocator.size_allocated() == 0;
          });
          if (removables != it.heaps.end())
          {
            it.heaps.erase(removables);
          }
          else
          {
            break;
          }
        }
      }

      auto removables = std::remove_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.heaps.empty();
      });
      if (removables != m_heaps.end())
      {
        m_heaps.erase(removables);
      }
      return emptyHeaps;
    }
  }
}