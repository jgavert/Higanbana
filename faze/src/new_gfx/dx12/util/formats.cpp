#include "formats.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
namespace faze
{
  namespace backend
  {
    static constexpr FormatDX12Conversion DXFormatTransformTable[] =
    {
      { DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,              FormatType::Unknown },
      { DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_UINT,    DXGI_FORMAT_R32G32B32A32_UINT,    FormatType::Uint32RGBA },
      { DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_UINT,       DXGI_FORMAT_R32G32B32_UINT,       FormatType::Uint32RGB },
      { DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_UINT,          DXGI_FORMAT_R32G32_UINT,          FormatType::Uint32RG },
      { DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,             DXGI_FORMAT_R32_UINT,             FormatType::Uint32 },
      { DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_FLOAT,   DXGI_FORMAT_R32G32B32A32_FLOAT,   FormatType::Float32RGBA },
      { DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_FLOAT,      DXGI_FORMAT_R32G32B32_FLOAT,      FormatType::Float32RGB },
      { DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_FLOAT,         DXGI_FORMAT_R32G32_FLOAT,         FormatType::Float32RG },
      { DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,            FormatType::Float32 },
      { DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_FLOAT,   DXGI_FORMAT_R16G16B16A16_FLOAT,   FormatType::Float16RGBA },
      { DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_FLOAT,         DXGI_FORMAT_R16G16_FLOAT,         FormatType::Float16RG },
      { DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,            FormatType::Float16 },
      { DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UINT,    DXGI_FORMAT_R16G16B16A16_UINT,    FormatType::Uint16RGBA },
      { DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UINT,          DXGI_FORMAT_R16G16_UINT,          FormatType::Uint16RG },
      { DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UINT,             DXGI_FORMAT_R16_UINT,             FormatType::Uint16 },
      { DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UNORM,   DXGI_FORMAT_R16G16B16A16_UNORM,   FormatType::Unorm16RGBA },
      { DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UNORM,         DXGI_FORMAT_R16G16_UNORM,         FormatType::Unorm16RG },
      { DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,            DXGI_FORMAT_R16_UNORM,            FormatType::Unorm16 },
      { DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_R8G8B8A8_UINT,        FormatType::Uint8RGBA },
      { DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UNORM,           DXGI_FORMAT_R8G8_UINT,            FormatType::Uint8RG },
      { DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UNORM,             DXGI_FORMAT_R8_UINT,              FormatType::Uint8 },
      { DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UNORM,             DXGI_FORMAT_R8_UNORM,              FormatType::Unorm8 },
      { DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_R8G8B8A8_SINT,        FormatType::Int8RGBA },
      { DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,       FormatType::Unorm8RGBA },
      { DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  FormatType::Unorm8SRGBA },
      { DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM,       FormatType::Unorm8BGRA },
      { DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,  FormatType::Unorm8SBGRA },
      { DXGI_FORMAT_R10G10B10A2_TYPELESS,   DXGI_FORMAT_R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,    FormatType::Unorm10RGB2A },
      { DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,             DXGI_FORMAT_R32_UINT,             FormatType::Raw32 },
      { DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_D32_FLOAT,            DXGI_FORMAT_R32_FLOAT,            FormatType::Depth32 },
      { DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_D32_FLOAT_S8X24_UINT, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, FormatType::Depth32_Stencil8 },
    };

    // This isn't accurate.
    FormatDX12Conversion dxformatToFazeFormat(DXGI_FORMAT format)
    {
      for (int i = 0; i < ArrayLength(DXFormatTransformTable); ++i)
      {
        if (DXFormatTransformTable[i].view == format
          || DXFormatTransformTable[i].storage == format
          || DXFormatTransformTable[i].raw == format)
        {
          return DXFormatTransformTable[i];
        }
      }
      return DXFormatTransformTable[0];
    }

    FormatDX12Conversion formatTodxFormat(FormatType format)
    {
      for (int i = 0; i < ArrayLength(DXFormatTransformTable); ++i)
      {
        if (DXFormatTransformTable[i].enm == format)
        {
          return DXFormatTransformTable[i];
        }
      }
      return DXFormatTransformTable[0];
    }
  }
}
#endif