#include "AllocStuff.hpp"
#include "core/src/global_debug.hpp"

#include <stdlib.h>

void* allocs::pfnAllocation(
  void*                                       pUserData,
  size_t                                      size, // bytes
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  allocs* info = reinterpret_cast<allocs*>(pUserData);
  info->normalUse.fetch_add(size, std::memory_order_relaxed);
#if defined(PLATFORM_WINDOWS)
  return _aligned_malloc(size, alignment);
#else
  return aligned_alloc(size, alignment);
#endif
};

void* allocs::pfnReallocation(
  void*                                       /*pUserData*/,
  void*                                       pOriginal,
  size_t                                      size,
#if defined(PLATFORM_WINDOWS)
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  return _aligned_realloc(pOriginal, size, alignment);
#else
  size_t                                      /*alignment*/,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  return realloc(pOriginal, size);
#endif
};

void allocs::pfnFree(
  void*                                       /*pUserData*/,
  void*                                       pMemory)
{
#if defined(PLATFORM_WINDOWS)
  _aligned_free(pMemory);
#else
  free(pMemory);
#endif
};

void allocs::pfnInternalAllocation(
  void*                                       pUserData,
  size_t                                      size,
  VkInternalAllocationType                    /*allocationType*/,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  allocs* info = reinterpret_cast<allocs*>(pUserData);
  auto tmp = info->internalUse.fetch_add(size, std::memory_order_relaxed);
  F_LOG("internal memory used: %u bytes", tmp);
};

void allocs::pfnInternalFree(
  void*                                       pUserData,
  size_t                                      size,
  VkInternalAllocationType                    /*allocationType*/,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  allocs* info = reinterpret_cast<allocs*>(pUserData);
  auto tmp = info->internalUse.fetch_sub(size, std::memory_order_relaxed);
  F_LOG("internal memory used: %u bytes", tmp);
};
