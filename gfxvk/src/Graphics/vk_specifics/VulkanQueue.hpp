#pragma once
#include "gfxvk/src/Graphics/vk_specifics/VulkanCmdBuffer.hpp"
#include "gfxvk/src/Graphics/vk_specifics/VulkanFence.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

class VulkanQueue
{
private:
  friend class VulkanGpuDevice;
  std::shared_ptr<vk::Queue>    m_queue;
  VulkanQueue(std::shared_ptr<vk::Queue> queue);
public:
  bool isValid();
  void submit(VulkanCmdBuffer& buffer, FenceImpl& fence);
};

using QueueImpl = VulkanQueue;