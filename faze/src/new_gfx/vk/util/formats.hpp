#pragma once

#include "faze/src/new_gfx/vk/vulkan.hpp"
#include "faze/src/new_gfx/desc/formats.hpp"
#include "core/src/system/helperMacros.hpp"

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