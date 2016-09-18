#include "VulkanCmdBuffer.hpp"

#include "core/src/global_debug.hpp"
#include "VulkanBuffer.hpp"

using namespace faze;

class BufferCopyPacket : public VulkanCommandPacket
{
private:
  std::shared_ptr<vk::Buffer> from;
  std::shared_ptr<vk::Buffer> to;
  CommandListVector<vk::BufferCopy> m_copyList;
public:

  BufferCopyPacket(LinearAllocator& allocator, std::shared_ptr<vk::Buffer> from, std::shared_ptr<vk::Buffer> to, MemView<vk::BufferCopy> copyList)
    : from(from)
    , to(to)
    , m_copyList(std::forward<decltype(copyList)>(MemView<vk::BufferCopy>(allocator.allocList<vk::BufferCopy>(copyList.size()), copyList.size())))
  {
    F_ASSERT(copyList, "Invalid copy commands");
    for (size_t i = 0; i < m_copyList.size(); i++)
    {
      m_copyList[i] = copyList[i];
    }
  }
  void execute(vk::CommandBuffer& buffer) override
  {
    vk::ArrayProxy<const vk::BufferCopy> proxy(static_cast<uint32_t>(m_copyList.size()), m_copyList.data());
    buffer.copyBuffer(*from, *to, proxy);
  }
};


VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer)), m_pool(std::forward<decltype(pool)>(pool)), m_closed(false)
  , m_commandList(LinearAllocator(1024*512))
{}

void VulkanCmdBuffer::resetList()
{
  if (m_closed)
  {
    m_closed = false;
  }
}
bool VulkanCmdBuffer::isValid()
{
  return m_cmdBuffer.get() != nullptr;
}
void VulkanCmdBuffer::close()
{
  m_closed = true;
}
bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}
void VulkanCmdBuffer::copy(VulkanBuffer from, VulkanBuffer to)
{
  vk::BufferCopy copy;
  m_commandList.insert<BufferCopyPacket>(from.m_resource, to.m_resource, copy);
}
