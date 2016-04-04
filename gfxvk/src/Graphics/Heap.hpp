#pragma once
#include "core/src/entity/bitfield.hpp"
#include "vk_specifics/VulkanHeap.hpp"
#include "HeapDescriptor.hpp"

#include <mutex>
#include <memory>

class ResourceHeap 
{
  friend class GpuDevice;
  constexpr static int64_t s_pageBlockCount = 32;
  constexpr static int64_t s_pageCount = 128 * s_pageBlockCount;
  using HeapBitfield = faze::Bitfield<s_pageBlockCount>;

  MemoryHeapImpl                m_resource;
  std::shared_ptr<HeapBitfield> m_pages; // 16*128 pages, enough? hopefully. atleast simple.
  std::shared_ptr<std::mutex>   m_mutex; // access control

  // should be procted by the mutex outside of this function
  int64_t checkIfFreeContiguousMemory(uint64_t blockCount)
  {
    F_ASSERT(blockCount <= s_pageCount, "Too many pages... %u > %u", blockCount, s_pageCount);

    // just do the idiot way and forloop the whole bitfield
    // TODO: profile if horribly slow. and wave hands harder
    size_t continuosPages = 0;
    auto& pages = *m_pages;
    for (size_t i = 0; i < s_pageCount; ++i)
    {
      if (pages.checkIdxBit(i))
      {
        continuosPages = 0;
      }
      else
      {
        ++continuosPages;
      }
      if (continuosPages == blockCount)
      {
        return i - continuosPages + 1; // this might have one off
      }
    }
    return -1; // Invalid!
  }

  // should be procted by the mutex outside of this function
  void markUsedPages(int64_t startIndex, int64_t size)
  {
    F_ASSERT(startIndex + size <= s_pageCount, "not enough pages available");
    auto& pages = *m_pages;
    for (int64_t i = startIndex; i < startIndex + size; ++i)
    {
      pages.setIdxBit(i);
    }
  }

public:

  ResourceHeap(MemoryHeapImpl impl)
    : m_resource{ impl }
    , m_pages{std::make_shared<HeapBitfield>()}
    , m_mutex{std::make_shared<std::mutex>()}
  {}

  int64_t allocatePages(uint64_t sizeInBytes)
  {
    if (m_resource.desc().m_sizeInBytes < sizeInBytes)
    {
      return -1;
    }
    std::lock_guard<std::mutex> guard(*m_mutex);
    auto blocks = sizeInBytes / m_resource.desc().m_alignment;
    blocks += ((sizeInBytes%m_resource.desc().m_alignment != 0) ? 1 : 0);
    auto startIndex = checkIfFreeContiguousMemory(blocks);
    if (startIndex != -1)
    {
      markUsedPages(startIndex, blocks);
      return startIndex * m_resource.desc().m_alignment;
    }
    return -1;
  }

  // freeing is threadfree since the results doesn't have to be immideatly available.
  void freePages(int64_t startIndex, int64_t size)
  {
    F_ASSERT(startIndex + size <= s_pageCount, "not enough pages available");
    auto& pages = *m_pages;
    for (int64_t i = startIndex; i < startIndex + size; ++i)
    {
      pages.clearIdxBit(i);
    }
  }

  MemoryHeapImpl& impl()
  {
    return m_resource;
  }

  HeapDescriptor desc()
  {
    return m_resource.desc();
  }

  bool isValid()
  {
    return true;
  }
};


