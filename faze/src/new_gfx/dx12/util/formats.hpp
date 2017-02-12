#pragma once

#include "faze/src/new_gfx/desc/formats.hpp"

#include "faze/src/new_gfx/dx12/dx12.hpp"

namespace faze
{
  namespace backend
  {
    struct FormatDX12Conversion
    {
      DXGI_FORMAT raw;
      DXGI_FORMAT storage;
      DXGI_FORMAT view;
      FormatType enm;
    };
    constexpr static int DXFormatTransformTableSize = 10;
    static FormatDX12Conversion DXFormatTransformTable[DXFormatTransformTableSize] =
    {
      { DXGI_FORMAT_UNKNOWN,               DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN,            FormatType::Unknown },
      { DXGI_FORMAT_R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_R32G32B32A32_UINT,  FormatType::Uint32x4 },
      { DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_UNORM,  DXGI_FORMAT_R16G16B16A16_FLOAT, FormatType::Float16x4 },
      { DXGI_FORMAT_R8G8B8A8_TYPELESS,     DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UINT,      FormatType::Uint8x4 },
      { DXGI_FORMAT_R8G8B8A8_TYPELESS,     DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UINT,      FormatType::Uint8x4_Srgb },
      { DXGI_FORMAT_B8G8R8A8_TYPELESS,     DXGI_FORMAT_B8G8R8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UINT,      FormatType::Uint8x4_Bgr },
      { DXGI_FORMAT_B8G8R8A8_TYPELESS,     DXGI_FORMAT_B8G8R8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UINT,      FormatType::Uint8x4_Sbgr },
      { DXGI_FORMAT_R32_TYPELESS,          DXGI_FORMAT_R32_UINT,            DXGI_FORMAT_R32_UINT,           FormatType::R32 },
      { DXGI_FORMAT_R32_TYPELESS,          DXGI_FORMAT_D32_FLOAT,           DXGI_FORMAT_R32_FLOAT,          FormatType::Depth32 },
      { DXGI_FORMAT_R8_TYPELESS,           DXGI_FORMAT_R8_UNORM,            DXGI_FORMAT_R8_UINT,            FormatType::Uint8 },
    };

    FormatDX12Conversion dxformatToFazeFormat(DXGI_FORMAT format);
    FormatDX12Conversion formatTodxFormat(FormatType format);
  }
}
