#include "AllocStuff.hpp"

void* allocs::pfnAllocation(
  void*                                       /*pUserData*/,
  size_t                                      size, // bytes
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
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
  info->superInt.fetch_add(size, std::memory_order_relaxed);
};

void allocs::pfnInternalFree(
  void*                                       pUserData,
  size_t                                      size,
  VkInternalAllocationType                    /*allocationType*/,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  allocs* info = reinterpret_cast<allocs*>(pUserData);
  info->superInt.fetch_sub(size, std::memory_order_relaxed);
};