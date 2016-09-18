#pragma once
#include <memory>



class LinearAllocator
{
private:
  std::unique_ptr<uint8_t[]> m_data;
  size_t m_current;

  uintptr_t calcAlignOffset(uintptr_t size, size_t alignment = 16)
  {
    return (alignment - (size & alignment)) % alignment;
  }

  uintptr_t privateAlloc(size_t size)
  {
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

class CommandPacket
{
private:
  CommandPacket* m_nextPacket;
public:
  CommandPacket()
    :m_nextPacket(nullptr)
  {}

  void setNextPacket(CommandPacket* packet)
  {
    m_nextPacket = packet;
  }
  CommandPacket* nextPacket()
  {
    return m_nextPacket;
  }

  virtual void execute() = 0;
  virtual ~CommandPacket() {}
};

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
  ~CommandList()
  {
    CommandPacket* current = m_firstPacket;
    while (current != nullptr)
    {
      CommandPacket* tmp = (*current).nextPacket();
      (*current).~CommandPacket();
      current = tmp;
    }
  }

  size_t size() { return m_size; }

  template <typename Type, typename... Args>
  Type& insert(Args&&... args)
  {
    Type* ptr = m_allocator.alloc<Type>();
    ptr = new (ptr) Type(std::forward<Args>(args)...);
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

  template <typename Func>
  void foreach(Func f)
  {
    CommandPacket* current = m_firstPacket;
    while (current != nullptr)
    {
      f(*current);
      current = current->nextPacket();
    }
  }
};
