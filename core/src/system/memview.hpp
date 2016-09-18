#pragma once

#include <inttypes.h>

namespace faze
{
  template <typename T>
  class MemView
  {
  private:
    T* m_ptr = nullptr;
    size_t m_size = 0;
  public:
    MemView() {}

    MemView(T* ptr, size_t size)
      : m_ptr(ptr)
      , m_size(size)
    {}

    MemView(T& elem)
      : m_ptr(&elem)
      , m_size(1)
    {}

    template <template <typename, typename ...> class Container, typename ...Args>
    MemView(Container<T, Args...>& s)
      : m_ptr(s.data())
      , m_size(s.size())
    {
    }

    const T* begin() const
    {
      return m_ptr;
    }
    T* begin() 
    {
      return m_ptr;
    }
    const T* end() const
    {
      return m_ptr + m_size;
    }
    T* end() 
    {
      return m_ptr + m_size;
    }


    T* data()
    {
      return begin();
    }

    size_t size() const
    {
      return m_size;
    }

    T& operator[](size_t i)
    {
      return m_ptr[i];
    }

    operator bool() const
    {
      return m_ptr != nullptr && m_size > 0;
    }
  };

  template <template <typename, typename ...> class Container, typename Elem, typename ...Args>
  MemView<uint8_t> containerAsBytes(Container<Elem, Args...>& s)
  {
    return MemView<uint8_t>(s.data(), s.size());
  }

  // actually quite useful
  template <template <typename, typename ...> class Container, typename Elem, typename ...Args>
  MemView<Elem> containerAsMemView(Container<Elem, Args...>& s)
  {
    return MemView<Elem>(s.data(), s.size());
  }

  template <typename targetElem, typename Elem>
  MemView<targetElem> reinterpret_memView(MemView<Elem>& s)
  {
    targetElem* begin = reinterpret_cast<targetElem*>(s.begin());
    targetElem* end = reinterpret_cast<targetElem*>(s.end());
    auto size = end - begin;
    return MemView<targetElem>(begin, size);
  }

  template <typename Elem>
  MemView<uint8_t> containerAsBytes(MemView<Elem>& s)
  {
    return reinterpret_memView<uint8_t>(s);
  }
};
