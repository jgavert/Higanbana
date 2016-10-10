#pragma once

#include "gfxvk/src/Graphics/Descriptors/Formats.hpp"

#include <vulkan/vulkan.hpp>


struct FormatVulkanConversion 
{
	vk::Format storage;
	vk::Format view;
	FormatType enm;
};

static FormatVulkanConversion formatToVkFormat[] =
{
	{, vk::Format::eUndefined, },
	{, vk::Format::eR32G32B32A32Uint, },
	{, vk::Format::eR32G32B32A32Sfloat, },
	{, vk::Format::eR32G32B32Uint, },
	{, vk::Format::eR32G32B32Sfloat, },
	{, vk::Format::eR8G8B8A8Uint, },
	{, vk::Format::eR8G8B8A8Unorm, },
	{, vk::Format::eR8G8B8A8Srgb, },
	{, vk::Format::eR32Sfloat, },
	{, vk::Format::eD32Sfloat, },
	{, vk::Format::eR8Uint, },
	{, vk::Format::eR8Unorm, },
};

namespace faze
{

};