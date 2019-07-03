#include "graphics/vk/util/AllocStuff.hpp"
#include "core/Platform/definitions.hpp"
#include "core/global_debug.hpp"

#include <stdlib.h>
#include <stdint.h>
#include <cstring>

// sizeBytes is the amount of private data I need.
namespace
{
  constexpr uint32_t PrivateAllocSizeBytes = sizeof(uint32_t) + sizeof(uint32_t);

  size_t calcOffset(size_t alignment)
  {
    return (PrivateAllocSizeBytes / alignment) * alignment + alignment;
  }
}
// alignment information is closest to ptr
// size is after that.
void getPtrInfo(void* ptr, uint32_t& size, uint32_t& alignment)
{
  uint32_t* porig = reinterpret_cast<uint32_t*>(ptr);
  uint32_t* porigSize = reinterpret_cast<uint32_t*>(porig - 1);
  alignment = *(porig - 1);
  size = *(porigSize - 1);
}

// stores size and alignment info close to orig pointer
// used to calculate original address
void* insertSizeAndReturn(void* ptr, uint32_t size, uint32_t alignment, uint32_t offset)
{
  uint8_t* retPtr = reinterpret_cast<uint8_t*>(ptr) + offset;
  uint32_t* porig = reinterpret_cast<uint32_t*>(retPtr) - 1;
  *(porig) = alignment;
  uint32_t* porigSize = porig - 1;
  *(porigSize) = size;
  return reinterpret_cast<void*>(retPtr);
}

bool isAligned(void* ptr, size_t alignment)
{
  uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
  return (p % static_cast<uintptr_t>(alignment)) == 0;
}

void* allocs::pfnAllocation(
  void*                                       pUserData,
  size_t                                      size, // bytes
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  allocs* info = reinterpret_cast<allocs*>(pUserData);
#if defined(FAZE_PLATFORM_WINDOWS)
  info->normalUse.fetch_add(size, std::memory_order_relaxed);
  return _aligned_malloc(size, alignment);
#else
  size_t offset = calcOffset(alignment);
  info->normalUse.fetch_add(size + offset, std::memory_order_relaxed);
  auto ptr = aligned_alloc(alignment, size + offset);
  return insertSizeAndReturn(ptr, static_cast<uint32_t>(size), static_cast<uint8_t>(alignment), offset);
#endif
};

void* allocs::pfnReallocation(
  void*                                       pUserData,
  void*                                       pOriginal,
  size_t                                      size,
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  allocs* info = reinterpret_cast<allocs*>(pUserData);
#if defined(FAZE_PLATFORM_WINDOWS)
  info->normalUse.fetch_add(size, std::memory_order_relaxed);
  return _aligned_realloc(pOriginal, size, alignment);
#else
  void* ptr = nullptr;
  if (pOriginal != nullptr)
  {
    uint32_t sizeOld = 0;
    uint32_t alignOld = 0;
    getPtrInfo(pOriginal, sizeOld, alignOld);
    size_t offset_last = calcOffset(alignOld);
    size_t offset = calcOffset(alignment);
    void* origPtr = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(pOriginal) - offset_last);
    ptr = realloc(origPtr, size + offset);
    if (!isAligned(ptr, alignment))
    {
      auto aligPtr = aligned_alloc(alignment, size + offset);
      auto realPtr = insertSizeAndReturn(aligPtr, size, alignment, offset);
      memcpy(realPtr, ptr, sizeOld);
      free(ptr);
      ptr = realPtr;
    }
    else
    {
      ptr = insertSizeAndReturn(ptr, size, alignment, offset);
    }
    info->normalUse.fetch_add(static_cast<int>(size + offset) - static_cast<int>(sizeOld + offset_last), std::memory_order_relaxed);
  }
  else
  {
    size_t offset = calcOffset(alignment);
    info->normalUse.fetch_add(size + offset, std::memory_order_relaxed);
    ptr = aligned_alloc(alignment, size + offset);
    ptr = insertSizeAndReturn(ptr, static_cast<uint32_t>(size), static_cast<uint8_t>(alignment), offset);
  }
  return ptr;
#endif
};

void allocs::pfnFree(
  void*                                       pUserData,
  void*                                       pMemory)
{
  if (pMemory == nullptr)
    return;
  allocs* info = reinterpret_cast<allocs*>(pUserData);
#if defined(FAZE_PLATFORM_WINDOWS)
  info->normalUse.fetch_sub(1, std::memory_order_relaxed);
  _aligned_free(pMemory);
#else
  uint32_t sizeOld = 0;
  uint32_t alignOld = 0;
  getPtrInfo(pMemory, sizeOld, alignOld);
  uint32_t offset_last = calcOffset(alignOld);
  info->normalUse.fetch_sub(sizeOld + offset_last, std::memory_order_relaxed);
  void* origPtr = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(pMemory) - offset_last);
  free(origPtr);
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
