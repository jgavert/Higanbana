#pragma once
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
    Float16x4,
    Uint8x4,
    Uint8x4_Srgb,
    Uint8x4_Bgr,
    Uint8x4_Sbgr,
    R32,
    Depth32,
    Uint8,
    Count
  };
  /* backend can know
  static size_t sizeOfFormat[] =
  {
  0,
  4 * 4,
  4 * 4,
  3 * 4,
  3 * 4,
  4 * 1,
  4 * 1,
  4 * 1,
  1 * 4,
  1 * 4,
  1 * 1,
  1 * 1
  };
  */
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
}