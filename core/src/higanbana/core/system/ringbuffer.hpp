#pragma once
#include <vector>
#include <array>
#include <algorithm>
#include <functional>
#include <cmath>

namespace higanbana
{

  template <class T, size_t ArraySize>
  class RingBuffer
  {
  public:
    RingBuffer() : m_buffer(ArraySize), m_ptr(0), m_filledSize(0) {}
    ~RingBuffer() {}

    void push_back(T value)
    {
      m_buffer[m_ptr] = value;
      moveptr();
      m_filledSize++;
      if (m_filledSize >= ArraySize)
      {
        m_filledSize = ArraySize - 1;
      }
    }

    T& get() { return m_buffer[m_ptr]; }
    T* data() const { return m_buffer; }
    size_t size() const { return ArraySize; }
    T& operator[](int index) { return m_buffer[getIndex(index)]; }
    const T& operator[](int index) const { return m_buffer[getIndex(index)]; }
    int start_ind() { return ArraySize + m_ptr - m_filledSize; }
    int end_ind() { return ArraySize + m_ptr - 1; }
    void forEach(std::function< void(T&) > apply)
    {
      for (int i = start_ind(); i < end_ind(); i++)
      {
        apply(m_buffer[getIndex(i)]);
      }
    }
  private:
    void moveptr() { m_ptr = getIndex(m_ptr + 1); }
    int getIndex(int ind) { return ind % ArraySize; }

    std::vector<T> m_buffer;
    int m_ptr;
    int m_filledSize;
  };
}
