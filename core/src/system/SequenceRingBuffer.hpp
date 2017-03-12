#pragma once
#include "core/src/system/SequenceRingRangeAllocator.hpp"
#include "core/src/global_debug.hpp"
#include <vector>

namespace faze
{
  template <typename T>
  class SequenceRingBuffer
  {
  public:
    SequenceRingBuffer()
    {
    }

    void emplace(T&& element)
    {
      allocator = SequenceRingRangeAllocator(allocator.rangeSize() + 1);
      m_data.emplace_back(std::move(element));
    }

    T& next(SeqNum num)
    {
      auto range = allocator.nextRange(num, 1);
      return m_data[range.start()];
    }

  private:
    std::vector<T> m_data;
    SequenceRingRangeAllocator allocator;
  };
 
}
