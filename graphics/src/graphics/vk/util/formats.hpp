#pragma once

#include "graphics/vk/vulkan.hpp"
#include "graphics/desc/formats.hpp"
#include "core/system/helperMacros.hpp"

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