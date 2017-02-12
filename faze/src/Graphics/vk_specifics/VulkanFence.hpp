#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

class VulkanFence 
{
private:
  friend class VulkanGpuDevice;
  friend class VulkanQueue;
  std::shared_ptr<vk::Fence> m_fence;
  VulkanFence(std::shared_ptr<vk::Fence> fence);
public:
  VulkanFence();
};

using FenceImpl = VulkanFence;