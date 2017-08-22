#pragma once
#include "core/src/system/helperMacros.hpp"
#include <stddef.h>

namespace faze
{
  enum class FormatDimension
  {
    Unknown,
    Buffer,
    Texture1D,
    Texture2D,
    Texture3D,
    TextureCube,
    Count
  };

  enum class FormatType
  {
    Unknown = 0,
    Uint32x4,
    Uint32x3,
    Uint32x2,
    Uint32,
    Float32x4,
    Float32x3,
    Float32x2,
    Float32,
    Float16x4,
    Float16x2,
    Float16,
    Unorm16x4,
    Unorm16x2,
    Unorm16,
    Uint16x4,
    Uint16x2,
    Uint16,
    Uint8x4,
    Uint8x2,
    Int8x4,
    Unorm8x4,
    Unorm8x4_Srgb,
    Unorm8x4_Bgr,
    Unorm8x4_Sbgr,
    Unorm10x3,
    Raw32,
    Depth32,
    Depth32_Stencil8,
    Uint8,
	Unorm8,
    Count
  };

  struct FormatSizeInfo
  {
    FormatType fm;
    int pixelSize;
    int componentCount;
  };

  FormatSizeInfo formatSizeInfo(FormatType format);
  int formatBitDepth(FormatType format);

  const char* formatToString(FormatType format);

  enum class ResourceUsage
  {
    Upload,
    Readback,
    GpuReadOnly,
    GpuRW,
    RenderTarget,
    DepthStencil,
    RenderTargetRW,
    DepthStencilRW,
    Count
  };

  enum class LoadOp
  {
    Load,
    Clear,
    DontCare
  };

  enum class StoreOp
  {
    Store,
    DontCare
  };
}