#include "higanbana/graphics/desc/formats.hpp"
#include <higanbana/core/math/utils.hpp>
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
  /*
  struct FormatSizeInfo
  {
    FormatType fm;
    int pixelSize;
    int componentCount;
  };
  */

  static constexpr FormatSizeInfo FormatToSizeTable[] =
  {
    { FormatType::Unknown, 0, 0, FormatTypeIdentifier::Unknown},
    { FormatType::Uint32RGBA, 16, 4, FormatTypeIdentifier::Unsigned },
    { FormatType::Uint32RGB, 12, 3, FormatTypeIdentifier::Unsigned},
    { FormatType::Uint32RG, 8, 2, FormatTypeIdentifier::Unsigned },
    { FormatType::Uint32, 4, 1, FormatTypeIdentifier::Unsigned },
    { FormatType::Int32, 4, 1, FormatTypeIdentifier::Signed },
    { FormatType::Float32RGBA, 16, 4, FormatTypeIdentifier::Float },
    { FormatType::Float32RGB, 12, 3, FormatTypeIdentifier::Float },
    { FormatType::Float32RG, 8, 2, FormatTypeIdentifier::Float },
    { FormatType::Float32, 4, 1, FormatTypeIdentifier::Float },
    { FormatType::Float16RGBA, 8, 4, FormatTypeIdentifier::Float },
    { FormatType::Float16RG, 4, 2, FormatTypeIdentifier::Float },
    { FormatType::Float16, 2, 1, FormatTypeIdentifier::Float },
    { FormatType::Unorm16RGBA, 8, 4, FormatTypeIdentifier::Unorm },
    { FormatType::Unorm16RG, 4, 2, FormatTypeIdentifier::Unorm },
    { FormatType::Unorm16, 2, 1, FormatTypeIdentifier::Unorm },
    { FormatType::Uint16RGBA, 8, 4, FormatTypeIdentifier::Unsigned },
    { FormatType::Uint16RG, 4, 2, FormatTypeIdentifier::Unsigned },
    { FormatType::Uint16, 2, 1, FormatTypeIdentifier::Unsigned },
    { FormatType::Uint8RGBA, 4, 4, FormatTypeIdentifier::Unsigned },
    { FormatType::Uint8RG, 2, 2, FormatTypeIdentifier::Unsigned },
    { FormatType::Uint8, 1, 1, FormatTypeIdentifier::Unsigned },
    { FormatType::Unorm8, 1, 1, FormatTypeIdentifier::Unorm },
    { FormatType::Int8RGBA, 4, 4, FormatTypeIdentifier::Signed },
    { FormatType::Unorm8RGBA, 4, 4, FormatTypeIdentifier::Unorm },
    { FormatType::Unorm8SRGBA, 4, 4, FormatTypeIdentifier::Unorm },
    { FormatType::Unorm8BGRA, 4, 4, FormatTypeIdentifier::Unorm },
    { FormatType::Unorm8SBGRA, 4, 4, FormatTypeIdentifier::Unorm },
    { FormatType::Unorm10RGB2A, 4, 4, FormatTypeIdentifier::Unorm },
    { FormatType::Raw32, 4, 1, FormatTypeIdentifier::Unknown },
    { FormatType::Depth32, 4, 1, FormatTypeIdentifier::Unknown },
    { FormatType::Depth32_Stencil8, 8, 1, FormatTypeIdentifier::Unknown },
  };

  FormatType formatToSRGB(FormatType format) {
    if (format == FormatType::Unorm8RGBA){
      return FormatType::Unorm8SRGBA;
    }else if (format == FormatType::Unorm8BGRA) {
      return FormatType::Unorm8SBGRA;
    }
    return format;
  }

  FormatType findFormat(int components, int bits, FormatTypeIdentifier pixelType)
  {
    auto wantedPixelSize = (bits / 8) * components;
    HIGAN_ASSERT(wantedPixelSize > 0, "Yes, please bigger than 0 pixel sizes thankyou, probably error");
    for (auto&& format : FormatToSizeTable)
    {
      if (format.pixelSize == wantedPixelSize && pixelType == format.pixelType && format.componentCount == components)
      {
        return format.fm;
      }
    }
    return FormatType::Unknown;
  }

  FormatSizeInfo formatSizeInfo(FormatType format)
  {
    for (int i = 0; i < ArrayLength(FormatToSizeTable); ++i)
    {
      if (FormatToSizeTable[i].fm == format)
      {
        return FormatToSizeTable[i];
      }
    }
    return FormatToSizeTable[0];
  }

  int formatBitDepth(FormatType format)
  {
    if (format == FormatType::Unorm10RGB2A)
    {
      return 10;
    }
    auto slot = FormatToSizeTable[0];
    for (int i = 0; i < ArrayLength(FormatToSizeTable); ++i)
    {
      if (FormatToSizeTable[i].fm == format)
      {
        slot = FormatToSizeTable[i];
        break;
      }
    }
    return (slot.pixelSize / slot.componentCount) * 8;
  }

  size_t sizeFormatRowPitch(int2 size, FormatType type) {
    constexpr size_t APIRowPitchAlignmentRequirement = 256;
    size_t tightRowPitch = size.x * formatSizeInfo(type).pixelSize;
    return roundUpMultiplePowerOf2(tightRowPitch, APIRowPitchAlignmentRequirement);
  }

  size_t sizeFormatSlicePitch(int2 size, FormatType type) {
    constexpr size_t APIRowPitchAlignmentRequirement = 256;
    size_t tightRowPitch = size.x * formatSizeInfo(type).pixelSize;
    size_t laxRowPitch = roundUpMultiplePowerOf2(tightRowPitch, APIRowPitchAlignmentRequirement);
    return laxRowPitch * static_cast<size_t>(size.y);
  }

  size_t sizeFormatRowPitch(int3 size, FormatType type) {
    return sizeFormatRowPitch(size.xy(), type);
  }

  size_t sizeFormatSlicePitch(int3 size, FormatType type) {
    return sizeFormatSlicePitch(size.xy(), type);
  }

  const char* formatToString(FormatType format)
  {
	  switch (format)
	  {
		case FormatType::Uint32RGBA:
			return "Uint32RGBA";
		case FormatType::Uint32RGB:
			return "Uint32RGB";
		case FormatType::Uint32RG:
			return "Uint32RG";
		case FormatType::Uint32:
			return "Uint32";
		case FormatType::Float32RGBA:
			return "Float32RGBA";
		case FormatType::Float32RGB:
			return "Float32RGB";
		case FormatType::Float32RG:
			return "Float32RG";
		case FormatType::Float32:
			return "Float32";
		case FormatType::Float16RGBA:
			return "Float16RGBA";
		case FormatType::Float16RG:
			return "Float16RG";
		case FormatType::Float16:
			return "Float16";
		case FormatType::Unorm16RGBA:
			return "Unorm16RGBA";
		case FormatType::Unorm16RG:
			return "Unorm16RG";
		case FormatType::Unorm16:
			return "Unorm16";
		case FormatType::Uint16RGBA:
			return "Uint16RGBA";
		case FormatType::Uint16RG:
			return "Uint16RG";
		case FormatType::Uint16:
			return "Uint16";
		case FormatType::Uint8RGBA:
			return "Uint8RGBA";
		case FormatType::Uint8RG:
			return "Uint8RG";
		case FormatType::Uint8:
			return "Uint8";
		case FormatType::Unorm8:
			return "Unorm8";
		case FormatType::Int8RGBA:
			return "Int8RGBA";
		case FormatType::Unorm8RGBA:
			return "Unorm8RGBA";
		case FormatType::Unorm8SRGBA:
			return "Unorm8SRGBA";
		case FormatType::Unorm8BGRA:
			return "Unorm8BGRA";
		case FormatType::Unorm8SBGRA:
			return "Unorm8SBGRA";
		case FormatType::Unorm10RGB2A:
			return "Unorm10RGB2A";
		case FormatType::Raw32:
			return "Raw32";
		case FormatType::Depth24:
			return "Depth24";
		case FormatType::Depth24_Stencil8:
			return "Depth24_Stencil8";
		case FormatType::Depth32:
			return "Depth32";
		case FormatType::Depth32_Stencil8:
			return "Depth32_Stencil8";
		case FormatType::Unknown:
		default:
			break;
	  }
	  return "Unknown";
  }
}