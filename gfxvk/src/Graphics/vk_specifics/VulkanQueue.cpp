#include "VulkanQueue.hpp"

VulkanQueue::VulkanQueue(std::shared_ptr<vk::Queue> queue, int familyIndex)
  :m_queue(queue)
	, m_index(familyIndex)
{
  
}

bool VulkanQueue::isValid()
{
  return m_queue.get() != nullptr;
}

void VulkanQueue::submit(VulkanCmdBuffer& gfx, FenceImpl& fence)
{
  gfx.close();
  auto info = vk::SubmitInfo()
    .setCommandBufferCount(1)
    .setPCommandBuffers(gfx.m_cmdBuffer.get());
  vk::ArrayProxy<const vk::SubmitInfo> proxy(info);
  m_queue->submit(proxy, *fence.m_fence);
}