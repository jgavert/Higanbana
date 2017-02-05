#pragma once
#include "core/src/system/RangeBlockAllocator.hpp"
#include <mutex>
#include <memory>

namespace faze
{
  struct PageBlock
  {
    int64_t offset;
    int64_t size;
  };

  class PageAllocator
  {
    int m_pageSize = 0;
    size_t m_sizeInPages = 0;
    RangeBlockAllocator m_allocator;
  public:
    PageAllocator(int pageSize, size_t sizeInPages)
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
      F_ASSERT(b.offset >= 0, "Invalid block released.");
      m_allocator.release(faze::RangeBlock{ b.offset / m_pageSize, b.size / m_pageSize });
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

