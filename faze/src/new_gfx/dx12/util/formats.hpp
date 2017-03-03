#pragma once

#include "faze/src/new_gfx/desc/formats.hpp"
#include "core/src/system/helperMacros.hpp"
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


    FormatDX12Conversion dxformatToFazeFormat(DXGI_FORMAT format);
    FormatDX12Conversion formatTodxFormat(FormatType format);
  }
}
