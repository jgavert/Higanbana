#pragma once

#include "core/src/system/memview.hpp"
#include "core/src/global_debug.hpp"
#include <memory>



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

template <typename T>
class CommandListVector
{
private:
  faze::MemView<T> m_view;
public:
 // CommandListVector() {}
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

  CommandListVector(CommandListVector&&) = default;
  CommandListVector(const CommandListVector&) = delete;
  CommandListVector& operator=(CommandListVector&&) = default;
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

template <typename CommandPacket>
class CommandList
{
private:
  LinearAllocator m_allocator;
  CommandPacket* m_firstPacket;
  CommandPacket* m_lastPacket;
  size_t m_size;
public:
  CommandList(LinearAllocator&& allocator)
    : m_allocator(std::forward<LinearAllocator>(allocator))
    , m_firstPacket(nullptr)
    , m_lastPacket(nullptr)
    , m_size(0)
  {}
  CommandList(CommandList&& obj) = default;
  CommandList(const CommandList& obj) = delete;

  CommandList& operator=(CommandList&& obj) = default;
  CommandList& operator=(const CommandList& obj) = delete;

  void hardClear()
  {
    CommandPacket* current = m_firstPacket;
    while (current != nullptr)
    {
      CommandPacket* tmp = (*current).nextPacket();
      (*current).~CommandPacket();
      current = tmp;
    }
    m_allocator.reset();
    m_firstPacket = nullptr;
    m_lastPacket = nullptr;
    m_size = 0; 
  }

  ~CommandList()
  {
    hardClear();
  }

  size_t size() { return m_size; }

  template <typename Type, typename... Args>
  Type& insert(Args&&... args)
  {
    Type* ptr = m_allocator.alloc<Type>();
    ptr = new (ptr) Type(m_allocator, std::forward<Args>(args)...);
    if (m_lastPacket != nullptr)
    {
      m_lastPacket->setNextPacket(ptr);
    }
    if (m_firstPacket == nullptr)
    {
      m_firstPacket = ptr;
    }
    m_lastPacket = ptr;
    m_size++;
    return *ptr;
  }

  LinearAllocator& allocator()
  {
    return m_allocator;
  }

  template <typename Func>
  void foreach(Func f)
  {
    CommandPacket* current = m_firstPacket;
    while (current != nullptr)
    {
      f(current);
      current = current->nextPacket();
    }
  }
};
