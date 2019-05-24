#pragma once
#include <stddef.h>
#include <inttypes.h>
#include <iterator>

namespace faze
{
  template <typename T>
  class MemView
  {
  private:
    size_t m_size = 0;
    T* m_ptr = nullptr;
  public:
    MemView() {}

    MemView(T* ptr, size_t size)
      : m_size(size)
      , m_ptr(ptr)
    {}

    MemView(T& elem)
      : m_size(1)
      , m_ptr(&elem)
    {}

    template <template <typename, typename ...> class Container, typename ...Args>
    MemView(Container<T, Args...>& s)
      : m_size(std::end(s) - std::begin(s))
      , m_ptr(m_size == 0 ? nullptr : std::addressof(*std::begin(s)))
    {
    }

    template <typename RndIter>
    MemView(RndIter first, size_t size)
      : m_size(size)
      , m_ptr(first)
    {
    }

    template <typename RndIter>
    MemView(RndIter first, RndIter last)
      : m_size(last - first)
      , m_ptr(m_size == 0 ? nullptr : std::addressof(*first))
    {
    }

    template <typename MemViewO>
    MemView(MemViewO &&c)
      : m_size(std::end(c) - std::begin(c))
      , m_ptr(m_size == 0 ? nullptr : std::addressof(*std::begin(c)))
    {
    }

    bool empty() const
    {
      return m_size == 0;
    }

    T* begin()
    {
      return m_ptr;
    }

    T* end()
    {
      return m_ptr + m_size;
    }

    T* data()
    {
      return begin();
    }

    const T* begin() const
    {
      return m_ptr;
    }

    const T* end() const
    {
      return m_ptr + m_size;
    }

    const T* data() const
    {
      return begin();
    }

    size_t size() const
    {
      return m_size;
    }

    size_t size_bytes() const
    {
      return m_size * sizeof(T);
    }

    T& operator[](size_t i)
    {
      return m_ptr[i];
    }

    const T& operator[](size_t i) const
    {
      return m_ptr[i];
    }

    operator bool() const
    {
      return m_ptr != nullptr && m_size > 0;
    }
  };

  template <typename T>
  MemView<T> makeMemView(T* ptr, size_t size)
  {
    return MemView<T>(ptr, size);
  }

  template <typename T>
  MemView<T> makeMemView(T& obj)
  {
    return MemView<T>(&obj, 1);
  }

  // actually quite useful
  template <template <typename, typename ...> class Container, typename Elem, typename ...Args>
  MemView<Elem> makeMemView(Container<Elem, Args...>& s)
  {
    return MemView<Elem>(s.begin(), s.end());
  }

  template <typename T>
  MemView<uint8_t> makeByteView(T* ptr, size_t size)
  {
    return MemView<uint8_t>(reinterpret_cast<uint8_t*>(ptr), size);
  }

  template <typename T>
  MemView<const uint8_t> makeByteView(const T* ptr, size_t size)
  {
    return MemView<const uint8_t>(reinterpret_cast<const uint8_t*>(ptr), size);
  }

  template <typename T>
  MemView<uint8_t> makeByteView(T& obj)
  {
    return MemView<uint8_t>(&obj, 1);
  }

  // actually quite useful
  template <template <typename, typename ...> class Container, typename Elem, typename ...Args>
  MemView<uint8_t> makeByteView(Container<Elem, Args...>& s)
  {
    return MemView<uint8_t>(s.begin(), s.end());
  }

  template <typename targetElem, typename Elem>
  MemView<targetElem> reinterpret_memView(MemView<Elem> s)
  {
    targetElem* begin = reinterpret_cast<targetElem*>(s.begin());
    targetElem* end = reinterpret_cast<targetElem*>(s.end());
    auto size = end - begin;
    return MemView<targetElem>(begin, size);
  }

  template <template <typename, typename ...> class Container, typename Elem, typename ...Args>
  size_t containerByteCount(Container<Elem, Args...>& s)
  {
    return makeByteView(s).size();
  }
};