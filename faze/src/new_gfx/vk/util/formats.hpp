#pragma once

#include "faze/src/new_gfx/desc/formats.hpp"

#include <vulkan/vulkan.hpp>

namespace faze
{
  namespace backend
  {
    struct FormatVulkanConversion
    {
      vk::Format storage;
      vk::Format view;
      FormatType enm;
    };
    constexpr static int VkFormatTransformTableSize = 10;
    static FormatVulkanConversion VkFormatTransformTable[VkFormatTransformTableSize] =
    {
      {vk::Format::eUndefined,          vk::Format::eUndefined,           FormatType::Unknown },
      {vk::Format::eR32G32B32A32Uint,   vk::Format::eR32G32B32A32Uint,    FormatType::Uint32x4 },
      {vk::Format::eR16G16B16A16Unorm,  vk::Format::eR16G16B16A16Sfloat,  FormatType::Float16x4 },
      {vk::Format::eR8G8B8A8Unorm,      vk::Format::eR8G8B8A8Uint,        FormatType::Uint8x4 },
      {vk::Format::eR8G8B8A8Unorm,      vk::Format::eR8G8B8A8Srgb,        FormatType::Uint8x4_Srgb },
      {vk::Format::eB8G8R8A8Unorm,      vk::Format::eB8G8R8A8Uint,        FormatType::Uint8x4_Bgr },
      {vk::Format::eB8G8R8A8Unorm,      vk::Format::eB8G8R8A8Srgb,        FormatType::Uint8x4_Sbgr },
      {vk::Format::eR32Uint,            vk::Format::eR32Uint,             FormatType::R32 },
      {vk::Format::eD32Sfloat,          vk::Format::eR32Sfloat,           FormatType::Depth32 },
      {vk::Format::eR8Unorm,            vk::Format::eR8Uint,              FormatType::Uint8 },
    };

    FormatVulkanConversion formatToFazeFormat(vk::Format format);
    FormatVulkanConversion formatToVkFormat(FormatType format);
  }
}
