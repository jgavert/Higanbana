#include "VulkanCmdBuffer.hpp"

#include "VulkanBuffer.hpp"

VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer)), m_pool(std::forward<decltype(pool)>(pool)), m_closed(false)
  , m_commandList(LinearAllocator(16386))
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

class CopyBufferToBuffer : public CommandPacket
{
private:
  std::shared_ptr<vk::Buffer> from;
  std::shared_ptr<vk::Buffer> to;
public:

  CopyBufferToBuffer(std::shared_ptr<vk::Buffer> from, std::shared_ptr<vk::Buffer> to)
    : from(from)
    , to(to)
  {
    
  }
  void execute() override
  {

  }
};

void VulkanCmdBuffer::copy(VulkanBuffer from, VulkanBuffer to)
{
  m_commandList.insert<CopyBufferToBuffer>(from.m_resource, to.m_resource);
}
