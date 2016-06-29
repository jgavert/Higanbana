#include "VulkanCmdBuffer.hpp"

VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer)), m_pool(std::forward<decltype(pool)>(pool)), m_closed(false)
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
void VulkanCmdBuffer::closeList()
{
  m_closed = true;
}
bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}
