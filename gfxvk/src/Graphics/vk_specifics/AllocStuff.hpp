#pragma once

#include <vulkan/vulkan.hpp>
#include <atomic>

class allocs
{
private:
  std::atomic<uint64_t> internalUse;
  std::atomic<uint64_t> normalUse;
public:
  static void* pfnAllocation(void*, size_t, size_t, VkSystemAllocationScope);
  static void* pfnReallocation(void* , void*, size_t, size_t, VkSystemAllocationScope);
  static void pfnFree( void*, void*);
  static void pfnInternalAllocation(void*, size_t, VkInternalAllocationType, VkSystemAllocationScope);
  static void pfnInternalFree( void*, size_t, VkInternalAllocationType, VkSystemAllocationScope);

  uint64_t getTotalBytesAlloc()
  {
    return normalUse.load(std::memory_order_relaxed);
  }

  uint64_t getTotalBytesAlloc_internal()
  {
    return normalUse.load(std::memory_order_relaxed);
  }
};
