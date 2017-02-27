#pragma once
#include "core/src/global_debug.hpp"
#include <memory>

namespace faze
{
  class LinearAllocator
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
    LinearAllocator(size_t size)
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
}