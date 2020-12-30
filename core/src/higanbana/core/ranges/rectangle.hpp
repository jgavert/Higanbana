#pragma once

#include "higanbana/core/math/math.hpp"

namespace higanbana { 
namespace ranges {

class Range2D {
public:
  Range2D() {}
  Range2D(uint2 size, uint2 stepSize):m_size(size), m_stepSize(stepSize) {}
private:
  uint2 m_size;
  uint2 m_stepSize;
public:

  class Range2DIterator 
  {
    uint2 m_current;
    uint2 m_stepSize;
    uint2 m_size;
    
  public:
    Range2DIterator() = default;
    Range2DIterator(const Range2DIterator&) = default;
    Range2DIterator& operator=(const Range2DIterator&) = default;

    Range2DIterator(uint2 start, uint2 stepSize, uint2 size)
        : m_current(start)
        , m_stepSize(stepSize)
        , m_size(size)
    {
    }

    Range2DIterator& operator++()
    {
      auto stepInX = std::min(m_stepSize.x, m_size.x-m_current.x);
      if (m_current.x+stepInX < m_size.x) {
        m_current.x += stepInX;
        return *this;
      }
      auto stepInY = std::min(m_stepSize.y, m_size.y-m_current.y);
      m_current.x = 0;
      if (m_current.y+stepInY < m_size.y) {
        m_current.y += stepInY;
        return *this;
      }
      m_current = m_size;
      return *this;
    }

    Rect operator*()
    {
      uint2 size;
      size.x = std::min(m_stepSize.x, m_size.x-m_current.x);
      size.y = std::min(m_stepSize.y, m_size.y-m_current.y);
      return Rect(m_current, size);
    }

    constexpr auto operator<=>(const Range2DIterator&) const = default;
  };
  Range2DIterator begin()
  {
    return Range2DIterator(uint2(0,0), m_stepSize, m_size);
  }

  Range2DIterator end()
  {
    return Range2DIterator(m_size, m_stepSize, m_size);
  }

  Range2DIterator cbegin() const
  {
    return Range2DIterator(uint2(0,0), m_stepSize, m_size);
  }

  Range2DIterator cend() const
  {
    return Range2DIterator(m_size, m_stepSize, m_size);
  }
};
}
}