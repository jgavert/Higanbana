#pragma once
#include <higanbana/core/platform/definitions.hpp>
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/desc/formats.hpp"
#include "higanbana/graphics/dx12/dx12.hpp"
#include <higanbana/core/system/helperMacros.hpp>

namespace higanbana
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

    FormatDX12Conversion dxformatToHiganbanaFormat(DXGI_FORMAT format);
    FormatDX12Conversion formatTodxFormat(FormatType format);
  }
}
#endif