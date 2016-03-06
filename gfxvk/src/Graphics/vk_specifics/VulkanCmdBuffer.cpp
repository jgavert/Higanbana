#include "VulkanCmdBuffer.hpp"

VulkanCmdBuffer::VulkanCmdBuffer(FazPtrVk<vk::CommandBuffer> buffer)
  : m_cmdBuffer(buffer), m_closed(false)
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
  return m_cmdBuffer.isValid();
}
void VulkanCmdBuffer::closeList()
{
  m_closed = true;
}
bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}
