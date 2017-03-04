#pragma once

#include <vulkan/vulkan.hpp>

class VulkanSurface
{
public:
#if defined(FAZE_PLATFORM_WINDOWS)
	std::shared_ptr<vk::SurfaceKHR> surface;
#else
	int surface;
#endif
};

using SurfaceImpl = VulkanSurface;