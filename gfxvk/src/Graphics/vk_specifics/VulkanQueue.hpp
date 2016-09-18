#pragma once
#include "VulkanCmdBuffer.hpp"

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
  void insertFence();
  void submit(VulkanCmdBuffer& buffer);
};

using QueueImpl = VulkanQueue;