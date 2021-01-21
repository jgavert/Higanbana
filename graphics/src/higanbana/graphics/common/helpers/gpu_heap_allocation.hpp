#pragma once

#include <higanbana/core/system/heap_allocator.hpp>

namespace higanbana
{
struct GpuHeapAllocation
{
  uint64_t index;
  int alignment;
  int64_t heapType;
  RangeBlock block;

  bool valid() { return alignment != -1 && index != static_cast<uint64_t>(-1); }
};
}