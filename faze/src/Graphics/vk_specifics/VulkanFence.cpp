#include "VulkanFence.hpp"

VulkanFence::VulkanFence(std::shared_ptr<vk::Fence> fence)
  : m_fence(fence)
{

}

VulkanFence::VulkanFence() {}