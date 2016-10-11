#pragma once
#include "gfxvk/src/Graphics/vk_specifics/VulkanCmdBuffer.hpp"
#include "gfxvk/src/Graphics/vk_specifics/VulkanFence.hpp"
#include "gfxvk/src/Graphics/vk_specifics/VulkanSemaphore.hpp"
#include "gfxvk/src/Graphics/vk_specifics/VulkanSwapchain.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

class VulkanQueue
{
private:
  friend class VulkanGpuDevice;
  std::shared_ptr<vk::Queue>    m_queue;
  int m_index = 0;
  VulkanQueue(std::shared_ptr<vk::Queue> queue, int index);
public:
  bool isValid();
  void submit(VulkanCmdBuffer& buffer, FenceImpl& fence);
  void submitWithSemaphore(VulkanCmdBuffer& gfx, FenceImpl& fence, VulkanSemaphore& waitImage, VulkanSemaphore& signalFinished);
  void present(VulkanSwapchain& sc, VulkanSemaphore& renderingFinished, unsigned currentImageIndice);
};

using QueueImpl = VulkanQueue;