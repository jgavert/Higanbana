#pragma once
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/helpers/gpu_heap_allocation.hpp"
#include <memory>

namespace higanbana 
{
class HeapDescriptor;
namespace backend
{
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
}
}