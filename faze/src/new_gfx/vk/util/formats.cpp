#include "formats.hpp"

namespace faze
{
  namespace backend
  {
    FormatVulkanConversion formatToFazeFormat(vk::Format format)
    {
      for (int i = 0; i < VkFormatTransformTableSize; ++i)
      {
        if (VkFormatTransformTable[i].view == format || VkFormatTransformTable[i].storage == format)
        {
          return VkFormatTransformTable[i];
        }
      }
      return VkFormatTransformTable[0];
    }

    FormatVulkanConversion formatToVkFormat(FormatType format)
    {
      for (int i = 0; i < VkFormatTransformTableSize; ++i)
      {
        if (VkFormatTransformTable[i].enm == format)
        {
          return VkFormatTransformTable[i];
        }
      }
      return VkFormatTransformTable[0];
    }
  }
}