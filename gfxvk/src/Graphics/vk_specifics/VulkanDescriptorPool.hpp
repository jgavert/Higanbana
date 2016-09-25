#pragma once

#include <vulkan/vulkan.hpp>

class VulkanDescriptorPool
{
private:
  vk::DescriptorPool pool;
public:
  VulkanDescriptorPool(vk::DescriptorPool pool)
    : pool(pool)
  {

  }
};