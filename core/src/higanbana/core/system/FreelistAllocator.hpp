#pragma once
#include "higanbana/core/datastructures/vector.hpp"
#include <mutex>
#include <memory>

namespace higanbana
{
class FreelistAllocator
{
  vector<int> m_values;
  int m_lastNewValue = 0;
public:
  FreelistAllocator()
  {

  }

  int allocate()
  {
    if (m_values.empty())
    {
      return m_lastNewValue++;
    }
    auto val = m_values.back();
    m_values.pop_back();
    return val;
  }

  void release(int val)
  {
    m_values.push_back(val);
  }

  bool empty() const noexcept
  {
    return m_values.empty();
  }

  size_t size() const noexcept
  {
    return m_values.size();
  }

  size_t max_size() const noexcept
  {
    return m_values.max_size();
  }
};
}

