#pragma once

#include "gfxvk/src/Graphics/Descriptors/Formats.hpp"

#include <vulkan/vulkan.hpp>


struct FormatVulkanConversion 
{
	vk::Format view;
	FormatType enm;
};

static FormatVulkanConversion formatToVkFormat[] =
{
	{vk::Format::eUndefined, FormatType::Unknown },
	{vk::Format::eR32G32B32A32Uint, FormatType::R32G32B32A32_Uint },
	{vk::Format::eR32G32B32A32Sfloat, FormatType::R32G32B32A32_Float },
	{vk::Format::eR32G32B32Uint, FormatType::R32G32B32_Uint },
	{vk::Format::eR32G32B32Sfloat, FormatType::R32G32B32_Float },
	{vk::Format::eR8G8B8A8Uint, FormatType::R8G8B8A8_Uint },
	{vk::Format::eR8G8B8A8Srgb, FormatType::R8G8B8A8_Srgb },
  {vk::Format::eB8G8R8A8Unorm, FormatType::B8G8R8A8 },
	{vk::Format::eR32Sfloat, FormatType::R32_Float },
	{vk::Format::eD32Sfloat, FormatType::D32 },
	{vk::Format::eR8Uint, FormatType::R8_Uint },
};

FormatType formatToFazeFormat(vk::Format format);
