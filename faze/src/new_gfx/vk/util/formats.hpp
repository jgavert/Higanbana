#pragma once

#include "faze/src/new_gfx/desc/formats.hpp"
#include "core/src/system/helperMacros.hpp"

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

    FormatVulkanConversion formatToFazeFormat(vk::Format format);
    FormatVulkanConversion formatToVkFormat(FormatType format);
  }
}
