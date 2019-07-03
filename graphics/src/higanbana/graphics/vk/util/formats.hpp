#pragma once

#include "higanbana/graphics/vk/vulkan.hpp"
#include "higanbana/graphics/desc/formats.hpp"
#include <higanbana/core/system/helperMacros.hpp>

namespace higanbana
{
  namespace backend
  {
    struct FormatVulkanConversion
    {
      vk::Format storage;
      vk::Format view;
      FormatType enm;
    };

    FormatVulkanConversion formatToHiganbanaFormat(vk::Format format);
    FormatVulkanConversion formatToVkFormat(FormatType format);
  }
}