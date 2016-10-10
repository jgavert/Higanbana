#include "VulkanFormat.hpp"

FormatType formatToFazeFormat(vk::Format format)
{
  for (auto&& it : formatToVkFormat)
  {
    if (it.view == format)
      return it.enm;
  }
  return FormatType::Unknown;
}
