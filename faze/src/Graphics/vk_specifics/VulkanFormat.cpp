#include "VulkanFormat.hpp"

FormatType formatToFazeFormat(vk::Format format)
{
  for (auto&& it : formatTransformTable)
  {
    if (it.view == format)
      return it.enm;
  }
  return FormatType::Unknown;
}


vk::Format formatToVkFormat(FormatType format)
{
  return formatTransformTable[format].view;
}

vk::Format formatToViewVkFormat(FormatType format)
{
  return formatTransformTable[format].view;
}

vk::Format formatToStorageVkFormat(FormatType format)
{
  return formatTransformTable[format].view;
}