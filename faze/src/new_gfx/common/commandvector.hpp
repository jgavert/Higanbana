#pragma once
#include "core/src/system/memview.hpp"
#include <memory>

namespace faze
{
  template <typename T>
  class CommandListVector
  {
  private:
    faze::MemView<T> m_view;
  public:
    CommandListVector()
    {
    }

    CommandListVector(faze::MemView<T> view)
      : m_view(std::forward<decltype(view)>(view))
    {
      if (m_view && !std::is_pod<T>::value)
      {
        for (size_t i = 0; i < view.size(); ++i)
        {
          new (view.data() + i) T();
        }
      }
    }

    CommandListVector(CommandListVector&& other)
      :m_view(std::move(other.m_view))
    {
      other.m_view = MemView<T>();
    }
    CommandListVector(const CommandListVector&) = delete;
    CommandListVector& operator=(CommandListVector&& other)
    {
      if (this != &other)
      {
        m_view = other.m_view;
        other.m_view = MemView<T>();
      }
      return *this;
    }

    CommandListVector& operator=(const CommandListVector&) = delete;

    ~CommandListVector()
    {
      if (m_view && !std::is_pod<T>::value)
      {
        for (size_t i = 0; i < m_view.size(); ++i)
        {
          m_view[i].~T();
        }
      }
    }

    T& operator[](size_t i)
    {
      return m_view.data()[i];
    }

    T* begin() const
    {
      return m_view.begin();
    }

    T* end() const
    {
      return m_view.end();
    }
    T* begin()
    {
      return m_view.begin();
    }

    T* end()
    {
      return m_view.end();
    }

    T* data() const
    {
      return m_view.data();
    }

    T* data()
    {
      return m_view.data();
    }

    size_t size() const
    {
      return m_view.size();
    }
  };
}