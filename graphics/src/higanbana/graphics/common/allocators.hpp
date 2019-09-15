#pragma once
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/global_debug.hpp>
#include <higanbana/core/math/utils.hpp>
#include <memory>

namespace higanbana
{
  class LinearMemoryAllocator
  {
  private:
    std::unique_ptr<uint8_t[]> m_data;
    size_t m_current;
    size_t m_size;

    inline uintptr_t privateAlloc(size_t size)
    {
	  int64_t alignedCurrent = roundUpMultipleInt(m_current, 16);
      HIGAN_ASSERT(alignedCurrent + size < m_size, "No space in allocator");
      if (size == 0)
        return 0;
      m_current = alignedCurrent + size;
      return reinterpret_cast<uintptr_t>(&m_data[alignedCurrent]);
    }

  public:
    LinearMemoryAllocator(size_t size)
      : m_data(std::make_unique<uint8_t[]>(size))
      , m_current(0)
      , m_size(size)
    {}

    template <typename T>
    inline T* alloc()
    {
      return reinterpret_cast<T*>(privateAlloc(sizeof(T)));
    }

    template <typename T>
    inline T* allocList(size_t count)
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

  public:
    LinearAllocator() {}
    LinearAllocator(size_t size)
	    : m_current(0)
      , m_size(static_cast<int64_t>(size))
    {}

    inline int64_t allocate(size_t size, size_t alignment = 1)
    {
      int64_t alignedCurrent = roundUpMultipleInt(m_current, alignment);
      if (alignedCurrent + static_cast<int64_t>(size) > m_size)
      {
        return -1;
      }
      m_current = alignedCurrent + static_cast<int64_t>(size);
      return alignedCurrent;
    }

    inline void resize(int64_t size)
    {
      m_size = size;
    }

    inline void reset()
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

    inline void grow()
    {
      m_freelist.emplace_back(m_size++);
    }

    inline int allocate()
    {
      if (m_freelist.empty())
      {
        return -1;
      }
      auto ret = m_freelist.back();
      m_freelist.pop_back();
      return ret;
    }

    inline void release(int val)
    {
      HIGAN_ASSERT(static_cast<int>(m_freelist.size()) != m_size
        && val >= 0 && val < m_size, "freelist already full, is this valid?");
      m_freelist.push_back(val);
    }
  };
}
