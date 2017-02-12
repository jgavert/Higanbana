#pragma once

#include <vulkan/vulkan.hpp>

class VulkanSemaphore
{
public:
  std::shared_ptr<vk::Semaphore> semaphore;
};

using SemaphoreImpl = VulkanSemaphore;