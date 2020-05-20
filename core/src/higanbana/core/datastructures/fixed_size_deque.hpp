#pragma once
#include <cstdint>
#include <type_traits>
#include <utility>

namespace higanbana
{
template<typename T, size_t N>
class FixedSizeDeque
{
  size_t m_count = 0;
  size_t m_head = 0;
  T m_arr[N] = {};

  size_t index_back() {
    //assert(m_count > 0);
    return (m_head + m_count-1) % N;
  }
  size_t index_front() {
    //assert(m_count > 0);
    return (m_head) % N;
  }
public:

  void push_back(T value) {
    m_count++;
    m_arr[index_back()] = value;
  }
  void push_front(T value) {
    if (m_head == 0)
      m_head = N;
    m_head--;
    m_arr[index_front()] = value;
    m_count++;
  }
  T& back() {
    return m_arr[index_back()];
  }
  T& front() {
    return m_arr[index_front()];
  }
  void pop_back() {
    m_arr[index_back()].~T();
    m_count--;
  }
  void pop_front() {
    m_arr[index_front()].~T();
    m_count--;
    m_head++;
    if (m_head == N)
      m_head = 0;
  }
  bool empty() const { return m_count==0; }
  size_t size() const { return m_count; }
};
}