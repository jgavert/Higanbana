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

void VulkanQueue::submitWithSemaphore(VulkanCmdBuffer& gfx, FenceImpl& fence, VulkanSemaphore& waitImage, VulkanSemaphore& signalFinished)
{
	gfx.close();

	vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eAllCommands;
	auto info = vk::SubmitInfo()
		.setPWaitDstStageMask(&waitMask)
		.setCommandBufferCount(1)
		.setPCommandBuffers(gfx.m_cmdBuffer.get())
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(waitImage.semaphore.get())
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(signalFinished.semaphore.get());
	vk::ArrayProxy<const vk::SubmitInfo> proxy(info);
	m_queue->submit(proxy, *fence.m_fence);
}

void VulkanQueue::present(VulkanSwapchain& sc, VulkanSemaphore& renderingFinished, unsigned currentImageIndice)
{
  m_queue->presentKHR(vk::PresentInfoKHR()
	.setSwapchainCount(1)
	.setPSwapchains(sc.m_swapchain.get())
	.setPImageIndices(&currentImageIndice)
	.setWaitSemaphoreCount(1)
	.setPWaitSemaphores(renderingFinished.semaphore.get()));
}