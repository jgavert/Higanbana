#pragma once
#include "core/src/global_debug.hpp"
#include <memory>

namespace faze
{
  class LinearMemoryAllocator
  {
  private:
    std::unique_ptr<uint8_t[]> m_data;
    size_t m_current;
    size_t m_size;

    uintptr_t calcAlignOffset(uintptr_t size, size_t alignment = 16)
    {
      return (alignment - (size & alignment)) % alignment;
    }

    uintptr_t privateAlloc(size_t size)
    {
      F_ASSERT(m_current + size < m_size, "No space in allocator");
      if (size == 0)
        return 0;
      auto freeMemory = m_current;
      m_current += size;
      uintptr_t ptrPos = reinterpret_cast<uintptr_t>(&m_data[freeMemory]);
      auto offset = calcAlignOffset(ptrPos);
      m_current += offset;
      ptrPos += offset;
      return ptrPos;
    }

  public:
    LinearMemoryAllocator(size_t size)
      : m_data(std::make_unique<uint8_t[]>(size))
      , m_current(0)
      , m_size(size)
    {}

    template <typename T>
    T* alloc()
    {
      return reinterpret_cast<T*>(privateAlloc(sizeof(T)));
    }

    template <typename T>
    T* allocList(size_t count)
    {
      return reinterpret_cast<T*>(privateAlloc(sizeof(T)*count));
    }

    inline void reset()
    {
      m_current = 0;
    }
  };

  class LinearAllocator
  {
  private:
    int64_t m_current = -1;
    int64_t m_size = -1;

    int64_t calcAlignOffset(int64_t size, size_t alignment)
    {
      return (alignment - (size & alignment)) % alignment;
    }
  public:
    LinearAllocator() {}
    LinearAllocator(size_t size)
      : m_size(static_cast<int64_t>(size))
    {}

    int64_t allocate(size_t size, size_t alignment = 1)
    {
      int64_t alignedCurrent = calcAlignOffset(m_current, alignment);
      if (alignedCurrent + static_cast<int64_t>(size) > m_size)
      {
        return -1;
      }
      m_current = alignedCurrent + static_cast<int64_t>(size);
      return alignedCurrent;
    }

    void resize(int64_t size)
    {
      m_size = size;
    }

    void reset()
    {
      m_current = 0;
    }
  };

  class FreelistAllocator
  {
    vector<int> m_freelist;
    int m_size = 0;
  public:
    FreelistAllocator()
    {}

    void grow()
    {
      m_freelist.emplace_back(m_size++);
    }

    int allocate()
    {
      if (m_freelist.empty())
      {
        return -1;
      }
      auto ret = m_freelist.back();
      m_freelist.pop_back();
      return ret;
    }

    void release(int val)
    {
      F_ASSERT(static_cast<int>(m_freelist.size()) != m_size
        && val >= 0 && val < m_size, "freelist already full, is this valid?");
      m_freelist.push_back(val);
    }
  };
}