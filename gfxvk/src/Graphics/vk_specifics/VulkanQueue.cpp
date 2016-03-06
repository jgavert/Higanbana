#include "VulkanQueue.hpp"

VulkanQueue::VulkanQueue(FazPtrVk<vk::Queue> queue)
  :m_queue(queue)
{
  
}

bool VulkanQueue::isValid()
{
  return m_queue.isValid();
}

void VulkanQueue::insertFence()
{

}
void VulkanQueue::submit(VulkanCmdBuffer& )
{

}