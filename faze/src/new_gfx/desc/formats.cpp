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
    {FormatType::Unknown, 0, 0},
    {FormatType::Uint32x4, 16, 4 },
    {FormatType::Uint32x3, 12, 3 },
    {FormatType::Uint32x2, 8, 2 },
    {FormatType::Uint32, 4, 1 },
    {FormatType::Float32x4, 16, 4 },
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
}