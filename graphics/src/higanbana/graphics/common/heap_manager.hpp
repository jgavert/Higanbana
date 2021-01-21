#pragma once

#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/helpers/gpu_heap_allocation.hpp"
#include "higanbana/graphics/common/helpers/heap_allocation.hpp"
#include <memory>

namespace higanbana
{
struct ResourceHandle;
class HeapDescriptor;
struct MemoryRequirements;

namespace backend
{

struct HeapAllocation;

class HeapManager
{
  struct HeapBlock
  {
    uint64_t index;
    HeapAllocator allocator;
    GpuHeap heap;
  };

  struct HeapVector
  {
    int64_t type;
    vector<HeapBlock> heaps;
  };

  vector<HeapVector> m_heaps;
  const int64_t m_minimumHeapSize = 16 * 1024 * 1024; // todo: move this configuration elsewhere
  uint64_t m_heapIndex = 0;

  uint64_t m_memoryAllocated = 0;
  uint64_t m_totalMemory = 0;
public:

  HeapAllocation allocate(MemoryRequirements requirements, std::function<GpuHeap(HeapDescriptor)> allocator);
  void release(GpuHeapAllocation allocation);
  vector<GpuHeap> emptyHeaps();
  uint64_t memoryInUse();
  uint64_t totalMemory();
};
}
}