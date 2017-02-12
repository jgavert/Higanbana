#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

class VulkanSwapchain
{
public:
	std::shared_ptr<vk::SwapchainKHR> m_swapchain;
};

using SwapchainImpl = VulkanSwapchain;