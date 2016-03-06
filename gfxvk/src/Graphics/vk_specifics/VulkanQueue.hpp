#pragma once
#include "VulkanCmdBuffer.hpp"

#include <vulkan/vk_cpp.h>

class VulkanQueue
{
private:
  friend class VulkanGpuDevice;
  FazPtrVk<vk::Queue>    m_queue;
  VulkanQueue(FazPtrVk<vk::Queue> queue);
public:
  bool isValid();
  void insertFence();
  void submit(VulkanCmdBuffer& buffer);
};

using QueueImpl = VulkanQueue;