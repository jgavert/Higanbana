#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>



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




class LinearAllocator
{
private:
  std::unique_ptr<uint8_t[]> m_data;
  size_t m_current;

  uintptr_t calcAlignOffset(uintptr_t size, size_t alignment = 16)
  {
    return (alignment - (size & alignment)) % alignment;
  }
public:
  LinearAllocator(size_t size)
    : m_data(std::make_unique<uint8_t[]>(size))
    , m_current(0)
  {}

  template <typename T, typename... Args>
  T* alloc(Args&&... args)
  {
    auto freeMemory = m_current;
    m_current += sizeof(T);
    uintptr_t ptrPos = reinterpret_cast<intptr_t>(&m_data[freeMemory]);
    auto offset = calcAlignOffset(ptrPos);
    m_current += offset;
    ptrPos += offset;
    T* ptr = new (reinterpret_cast<uint8_t*>(ptrPos)) T(std::forward<Args>(args)...);
    return ptr;
  }
  inline void reset()
  {
    m_current = 0;
  }
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
    Type* ptr = m_allocator.alloc<Type>(std::forward<Args>(args)...);
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

class VulkanBuffer;
class VulkanTexture;

class VulkanCmdBuffer
{
private:
  friend class VulkanGpuDevice;
  friend class VulkanQueue;
  std::shared_ptr<vk::CommandBuffer>   m_cmdBuffer;
  std::shared_ptr<vk::CommandPool>     m_pool;
  bool                          m_closed;
  CommandList                      m_commandList;
  VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool);
public:
  // Binding!?!?!?!?, hau, needs pipeline, needs binding.

  // copy
  void copy(VulkanBuffer from, VulkanBuffer to);
  // compute
  void dispatch();
  // draw

  void resetList();
  bool isValid();
  void close();
  bool isClosed();
};

using CmdBufferImpl = VulkanCmdBuffer;
