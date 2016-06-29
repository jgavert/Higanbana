#include "VulkanQueue.hpp"

VulkanQueue::VulkanQueue(std::shared_ptr<vk::Queue> queue)
  :m_queue(queue)
{
  
}

bool VulkanQueue::isValid()
{
  return m_queue.get() != nullptr;
}

void VulkanQueue::insertFence()
{

}
void VulkanQueue::submit(VulkanCmdBuffer& )
{

}