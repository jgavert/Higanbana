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
    { FormatType::Uint32x4, 16, 4 },
    { FormatType::Uint32x3, 12, 3 },
    { FormatType::Uint32x2, 8, 2 },
    { FormatType::Uint32, 4, 1 },
    { FormatType::Float32x4, 16, 4 },
    { FormatType::Float32x3, 12, 3 },
    { FormatType::Float32x2, 8, 2 },
    { FormatType::Float32, 4, 1 },
    { FormatType::Float16x4, 8, 4 },
    { FormatType::Float16x2, 4, 2 },
    { FormatType::Float16, 2, 1 },
    { FormatType::Unorm16x4, 8, 4 },
    { FormatType::Unorm16x2, 4, 2 },
    { FormatType::Unorm16, 2, 1 },
    { FormatType::Uint16x4, 8, 4 },
    { FormatType::Uint16x2, 4, 2 },
    { FormatType::Uint16, 2, 1 },
    { FormatType::Uint8x4, 4, 4 },
    { FormatType::Uint8x2, 2, 2 },
    { FormatType::Uint8, 1, 1 },
    { FormatType::Unorm8, 1, 1 },
    { FormatType::Int8x4, 4, 4 },
    { FormatType::Unorm8x4, 4, 4 },
    { FormatType::Unorm8x4_Srgb, 4, 4 },
    { FormatType::Unorm8x4_Bgr, 4, 4 },
    { FormatType::Unorm8x4_Sbgr, 4, 4 },
    { FormatType::Unorm10x3, 4, 4 },
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
    if (format == FormatType::Unorm10x3)
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
		case FormatType::Uint32x4:
			return "Uint32x4";
		case FormatType::Uint32x3:
			return "Uint32x3";
		case FormatType::Uint32x2:
			return "Uint32x2";
		case FormatType::Uint32:
			return "Uint32";
		case FormatType::Float32x4:
			return "Float32x4";
		case FormatType::Float32x3:
			return "Float32x3";
		case FormatType::Float32x2:
			return "Float32x2";
		case FormatType::Float32:
			return "Float32";
		case FormatType::Float16x4:
			return "Float16x4";
		case FormatType::Float16x2:
			return "Float16x2";
		case FormatType::Float16:
			return "Float16";
		case FormatType::Unorm16x4:
			return "Unorm16x4";
		case FormatType::Unorm16x2:
			return "Unorm16x2";
		case FormatType::Unorm16:
			return "Unorm16";
		case FormatType::Uint16x4:
			return "Uint16x4";
		case FormatType::Uint16x2:
			return "Uint16x2";
		case FormatType::Uint16:
			return "Uint16";
		case FormatType::Uint8x4:
			return "Uint8x4";
		case FormatType::Uint8x2:
			return "Uint8x2";
		case FormatType::Uint8:
			return "Uint8";
		case FormatType::Unorm8:
			return "Unorm8";
		case FormatType::Int8x4:
			return "Int8x4";
		case FormatType::Unorm8x4:
			return "Unorm8x4";
		case FormatType::Unorm8x4_Srgb:
			return "Unorm8x4_Srgb";
		case FormatType::Unorm8x4_Bgr:
			return "Unorm8x4_Bgr";
		case FormatType::Unorm8x4_Sbgr:
			return "Unorm8x4_Sbgr";
		case FormatType::Unorm10x3:
			return "Unorm10x3";
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