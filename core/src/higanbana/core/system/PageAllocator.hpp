#pragma once
#include "higanbana/core/system/RangeBlockAllocator.hpp"
#include <mutex>
#include <memory>

namespace higanbana
{
  struct PageBlock
  {
    int64_t offset;
    int64_t size;

    bool valid() const
    {
      return offset != -1 && size > 0;
    }
  };

  class FixedSizeAllocator
  {
    int64_t m_pageSize = 0;
    size_t m_sizeInPages = 0;
    RangeBlockAllocator m_allocator;
  public:
    FixedSizeAllocator(int64_t pageSize, size_t sizeInPages)
      : m_pageSize(pageSize)
      , m_sizeInPages(sizeInPages)
      , m_allocator(sizeInPages)
    {

    }

    PageBlock allocate(size_t bytes)
    {
      auto sizeInBlocks = sizeInPages(bytes);
      auto block = m_allocator.allocate(sizeInBlocks);
      if (block.offset == -1)
        return PageBlock{ -1, -1 };
      return PageBlock{ block.offset*m_pageSize, static_cast<int64_t>(sizeInBlocks)*m_pageSize };
    }

    void release(PageBlock b)
    {
      HIGAN_ASSERT(b.offset >= 0, "Invalid block released.");
      m_allocator.release(higanbana::RangeBlock{ b.offset / m_pageSize, b.size / m_pageSize });
    }

    size_t pageSize()
    {
      return static_cast<size_t>(m_pageSize);
    }

    bool empty()
    {
      return freesize() == size();
    }

    size_t size()
    {
      return m_sizeInPages * static_cast<size_t>(m_pageSize);
    }

    size_t freesize()
    {
      return m_allocator.freespace() * static_cast<size_t>(m_pageSize);
    }

    size_t freeContinuosSize()
    {
      return m_allocator.largestFreeBlockSize() * static_cast<size_t>(m_pageSize);
    }

  private:
    size_t sizeInPages(size_t count)
    {
      auto pages = count / m_pageSize;
      pages += ((count % m_pageSize != 0) ? 1 : 0);
      return pages;
    }
  };
}

