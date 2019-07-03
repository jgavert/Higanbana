#include "higanbana/graphics/vk/util/formats.hpp"

namespace higanbana
{
  namespace backend
  {
    static constexpr FormatVulkanConversion VkFormatTransformTable[] =
    {
      { vk::Format::eUndefined,           vk::Format::eUndefined,             FormatType::Unknown },
      { vk::Format::eR32G32B32A32Uint,    vk::Format::eR32G32B32A32Uint,      FormatType::Uint32RGBA },
      { vk::Format::eR32G32B32Uint,       vk::Format::eR32G32B32Uint,         FormatType::Uint32RGB },
      { vk::Format::eR32G32Uint,          vk::Format::eR32G32Uint,            FormatType::Uint32RG },
      { vk::Format::eR32Uint,             vk::Format::eR32Uint,               FormatType::Uint32 },
      { vk::Format::eR32G32B32A32Sfloat,  vk::Format::eR32G32B32A32Sfloat,    FormatType::Float32RGBA },
      { vk::Format::eR32G32B32Sfloat,     vk::Format::eR32G32B32Sfloat,       FormatType::Float32RGB },
      { vk::Format::eR32G32Sfloat,        vk::Format::eR32G32Sfloat,          FormatType::Float32RG },
      { vk::Format::eR32Sfloat,           vk::Format::eR32Sfloat,             FormatType::Float32 },
      { vk::Format::eR16G16B16A16Sfloat,  vk::Format::eR16G16B16A16Sfloat,    FormatType::Float16RGBA },
      { vk::Format::eR16G16Sfloat,        vk::Format::eR16G16Sfloat,          FormatType::Float16RG },
      { vk::Format::eR16Sfloat,           vk::Format::eR16Sfloat,             FormatType::Float16 },
      { vk::Format::eR16G16B16A16Uint,    vk::Format::eR16G16B16A16Uint,      FormatType::Uint16RGBA },
      { vk::Format::eR16G16Uint,          vk::Format::eR16G16Uint,            FormatType::Uint16RG },
      { vk::Format::eR16Uint,             vk::Format::eR16Uint,               FormatType::Uint16 },
      { vk::Format::eR16G16B16A16Unorm,   vk::Format::eR16G16B16A16Unorm,     FormatType::Unorm16RGBA },
      { vk::Format::eR16G16Unorm,         vk::Format::eR16G16Unorm,           FormatType::Unorm16RG },
      { vk::Format::eR16Unorm,            vk::Format::eR16Unorm,              FormatType::Unorm16 },
      { vk::Format::eR8G8B8A8Uint,        vk::Format::eR8G8B8A8Uint,          FormatType::Uint8RGBA },
      { vk::Format::eR8G8Uint,            vk::Format::eR8G8Uint,              FormatType::Uint8RG },
      { vk::Format::eR8Uint,              vk::Format::eR8Uint,                FormatType::Uint8 },
      { vk::Format::eR8Unorm,              vk::Format::eR8Unorm,                FormatType::Unorm8 },
      { vk::Format::eR8G8B8A8Sint,        vk::Format::eR8G8B8A8Sint,          FormatType::Int8RGBA },
      { vk::Format::eR8G8B8A8Unorm,       vk::Format::eR8G8B8A8Unorm,         FormatType::Unorm8RGBA },
      { vk::Format::eR8G8B8A8Srgb,        vk::Format::eR8G8B8A8Srgb,          FormatType::Unorm8SRGBA },
      { vk::Format::eB8G8R8A8Unorm,       vk::Format::eB8G8R8A8Unorm,         FormatType::Unorm8BGRA },
      { vk::Format::eB8G8R8A8Srgb,        vk::Format::eB8G8R8A8Srgb,          FormatType::Unorm8SBGRA },
      { vk::Format::eA2R10G10B10UnormPack32,   vk::Format::eA2R10G10B10UnormPack32,    FormatType::Unorm10RGB2A },
      { vk::Format::eR32Uint,             vk::Format::eR32Uint,               FormatType::Raw32 },
      { vk::Format::eD32Sfloat,           vk::Format::eR32Sfloat,             FormatType::Depth32 },
      { vk::Format::eD32SfloatS8Uint,     vk::Format::eR32Sfloat,             FormatType::Depth32_Stencil8 },
    };

    FormatVulkanConversion formatToHiganbanaFormat(vk::Format format)
    {
      for (int i = 0; i < ArrayLength(VkFormatTransformTable); ++i)
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
      for (int i = 0; i < ArrayLength(VkFormatTransformTable); ++i)
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