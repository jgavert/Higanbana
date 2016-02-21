#include "AllocStuff.hpp"
#include "core/src/global_debug.hpp"

void* allocs::pfnAllocation(
  void*                                       pUserData,
  size_t                                      size, // bytes
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  allocs* info = reinterpret_cast<allocs*>(pUserData);
  info->normalUse.fetch_add(size, std::memory_order_relaxed);
  return _aligned_malloc(size, alignment);
};

void* allocs::pfnReallocation(
  void*                                       /*pUserData*/,
  void*                                       pOriginal,
  size_t                                      size,
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  return _aligned_realloc(pOriginal, size, alignment);
};

void allocs::pfnFree(
  void*                                       /*pUserData*/,
  void*                                       pMemory)
{
  _aligned_free(pMemory);
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