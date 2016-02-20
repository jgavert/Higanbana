#pragma once

#include <vulkan/vk_cpp.h>
#include <atomic>

class allocs
{
private:
  std::atomic<uint64_t> superInt;
public:
  static void* pfnAllocation(void*, size_t, size_t, VkSystemAllocationScope);
  static void* pfnReallocation(void* , void*, size_t, size_t, VkSystemAllocationScope);
  static void pfnFree( void*, void*);
  static void pfnInternalAllocation(void*, size_t, VkInternalAllocationType, VkSystemAllocationScope);
  static void pfnInternalFree( void*, size_t, VkInternalAllocationType, VkSystemAllocationScope);
};
