#pragma once
#include "core/entity/bitfield.hpp"
#include "core/global_debug.hpp"

#include <mutex>
#include <memory>
#include <algorithm>

namespace faze
{
  struct RangeBlock
  {
    int64_t offset;
    int64_t size;
  };

  // 1 block is 128size, so 32*128 * 64KB == 256MB of space available. wastes 128/8 == 16bytes. so 32 blocks are 512bytes.
  template <int64_t internalPageCount>
  class RangeBlockAllocatorInternal
  {
    constexpr static int64_t s_pageBlockCount = internalPageCount;
    constexpr static int64_t s_pageCount = 128 * s_pageBlockCount;

    Bitfield<s_pageBlockCount> m_blocks; // 16*128 pages, enough? hopefully. atleast simple.
    std::mutex   m_mutex; // access control
                                           // should be procted by the mutex outside of this function
    int m_blockSize = 0;
    int64_t m_size = 0;

  public:

    RangeBlockAllocatorInternal(int blockSize, size_t amountOfBlocks)
      : m_blockSize(blockSize)
      , m_size(blockSize * amountOfBlocks)
    {}

    RangeBlock allocate(size_t amount)
    {
      if (static_cast<size_t>(m_size) < amount)
      {
        return RangeBlock{ -1, -1 };
      }
      std::lock_guard<std::mutex> guard(m_mutex);
      auto blocks = amountInBlocks(amount);
      auto startIndex = checkIfFreeContiguousMemory(blocks);
      if (startIndex != -1 && startIndex + blocks < amountInBlocks(m_size))
      {
        markUsedPages(startIndex, blocks);
        return RangeBlock{ startIndex * m_blockSize, blocks * m_blockSize };
      }
      return RangeBlock{ -1, -1 };
    }

    void release(RangeBlock range)
    {
      auto startBlock = amountInBlocks(range.offset);
      auto size = amountInBlocks(range.size);
      freeBlocks(startBlock, size);
    }

  private:

    int64_t amountInBlocks(size_t amount)
    {
      auto blocks = amount / m_blockSize;
      blocks += ((amount % m_blockSize != 0) ? 1 : 0);
      return blocks;
    }

    void freeBlocks(int64_t startBlock, int64_t blockCount)
    {
      F_ASSERT(startBlock + blockCount <= s_pageCount, "not enough implementation pages available");

      for (int64_t i = startBlock; i < startBlock + blockCount; ++i)
      {
        m_blocks.clearIdxBit(i);
      }
    }

    int64_t checkIfFreeContiguousMemory(uint64_t blockCount)
    {
      F_ASSERT(blockCount <= s_pageCount, "Too many pages... %u > %u", blockCount, s_pageCount);

      // just do the idiot way and forloop the whole bitfield
      // TODO: profile if horribly slow. and wave hands harder
      size_t continuosPages = 0;
      for (size_t i = 0; i < s_pageCount; ++i)
      {
        if (m_blocks.checkIdxBit(i))
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
      for (int64_t i = startIndex; i < startIndex + size; ++i)
      {
        m_blocks.setIdxBit(i);
      }
    }
  };

  //using RangeBlockAllocator = RangeBlockAllocatorInternal<32>;

  class RangeBlockAllocatorInternal2
  {
    DynamicBitfield m_blocks; // uses this invertly, setBit means that it's free for taking.
    int64_t m_size = 0;
    int64_t m_freespace = 0;
  public:
    RangeBlockAllocatorInternal2()
    {}
    RangeBlockAllocatorInternal2(size_t size)
      : m_blocks(size)
      , m_size(size)
      , m_freespace(size)
    {
      m_blocks.initFull();
    }

    RangeBlock allocate(size_t blocks)
    {
      int64_t inputBlocks = static_cast<int64_t>(blocks);
      if (m_size< inputBlocks)
      {
        return RangeBlock{ -1, -1 };
      }
      auto startIndex = checkIfFreeContiguousMemory(inputBlocks);
      if (startIndex != -1 && startIndex + static_cast<int64_t>(inputBlocks) <= m_size)
      {
        m_freespace -= inputBlocks;
        markUsedPages(startIndex, inputBlocks);
        return RangeBlock{ startIndex , inputBlocks };
      }
      return RangeBlock{ -1, -1 };
    }

    void release(RangeBlock range)
    {
      F_ASSERT(range.offset + range.size <= m_size, "not enough pages available");
      for (int64_t i = range.offset; i < range.offset + range.size; ++i)
      {
        m_blocks.setBit(i);
      }
      m_freespace += range.size;
    }

    size_t freespace()
    {
      return static_cast<size_t>(m_freespace);
    }

    size_t largestFreeBlockSize()
    {
      int64_t largestSize = 0;
      int64_t continuosPages = 0;
      for (int64_t i = 0; i < m_size; ++i)
      {
        if (!m_blocks.checkBit(i))
        {
          largestSize = std::max(largestSize, continuosPages);
          continuosPages = 0;
        }
        else
        {
          ++continuosPages;
        }
      }
      return largestSize;
    }

    size_t countContinuosFreeBlocks()
    {
      int64_t blocks = 0;
      int64_t continuosPages = 0;
      for (int64_t i = 0; i < m_size; ++i)
      {
        if (!m_blocks.checkBit(i))
        {
          if (continuosPages > 0)
            blocks++;
          continuosPages = 0;
        }
        else
        {
          ++continuosPages;
        }
      }
      return blocks;
    }

  private:
    int64_t checkIfFreeContiguousMemory(int64_t blockCount)
    {
      F_ASSERT(blockCount <= m_size, "Too many pages... %u > %u", blockCount, m_size);

      // just do the idiot way and forloop the whole bitfield
      // TODO: profile if horribly slow. and wave hands harder
      int64_t continuosPages = 0;
      for (int64_t i = 0; i < m_size; ++i)
      {
        if (!m_blocks.checkBit(i))
        {
          continuosPages = 0;
        }
        else
        {
          ++continuosPages;
        }
        if (continuosPages == blockCount)
        {
          return i - continuosPages+1; // this might have one off
        }
      }
      return -1; // Invalid!
    }

    // should be procted by the mutex outside of this function
    void markUsedPages(int64_t startIndex, int64_t size)
    {
      F_ASSERT(startIndex + size <= m_size, "not enough pages available");
      for (int64_t i = startIndex; i < startIndex + size; ++i)
      {
        m_blocks.clearBit(i);
      }
    }
  };

  using RangeBlockAllocator = RangeBlockAllocatorInternal2;
}

