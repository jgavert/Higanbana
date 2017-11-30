#include "formats.hpp"

namespace faze
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
    { FormatType::Unknown, 0, 0},
    { FormatType::Uint32RGBA, 16, 4 },
    { FormatType::Uint32RGB, 12, 3 },
    { FormatType::Uint32RG, 8, 2 },
    { FormatType::Uint32, 4, 1 },
    { FormatType::Float32RGBA, 16, 4 },
    { FormatType::Float32RGB, 12, 3 },
    { FormatType::Float32RG, 8, 2 },
    { FormatType::Float32, 4, 1 },
    { FormatType::Float16RGBA, 8, 4 },
    { FormatType::Float16RG, 4, 2 },
    { FormatType::Float16, 2, 1 },
    { FormatType::Unorm16RGBA, 8, 4 },
    { FormatType::Unorm16RG, 4, 2 },
    { FormatType::Unorm16, 2, 1 },
    { FormatType::Uint16RGBA, 8, 4 },
    { FormatType::Uint16RG, 4, 2 },
    { FormatType::Uint16, 2, 1 },
    { FormatType::Uint8RGBA, 4, 4 },
    { FormatType::Uint8RG, 2, 2 },
    { FormatType::Uint8, 1, 1 },
    { FormatType::Unorm8, 1, 1 },
    { FormatType::Int8RGBA, 4, 4 },
    { FormatType::Unorm8RGBA, 4, 4 },
    { FormatType::Unorm8SRGBA, 4, 4 },
    { FormatType::Unorm8BGRA, 4, 4 },
    { FormatType::Unorm8SBGRA, 4, 4 },
    { FormatType::Unorm10RGB2A, 4, 4 },
    { FormatType::Raw32, 4, 1 },
    { FormatType::Depth32, 4, 1 },
    { FormatType::Depth32_Stencil8, 8, 1 },
  };

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