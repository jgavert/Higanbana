#pragma once
#include "core/platform/definitions.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
#include "graphics/desc/formats.hpp"
#include "core/system/helperMacros.hpp"
#include "graphics/dx12/dx12.hpp"

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

    FormatDX12Conversion dxformatToFazeFormat(DXGI_FORMAT format);
    FormatDX12Conversion formatTodxFormat(FormatType format);
  }
}
#endif