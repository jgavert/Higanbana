#pragma once
#include <higanbana/core/system/helperMacros.hpp>

namespace higanbana
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

  enum class FormatTypeIdentifier
  {
    Unknown = 0,
    Unsigned,
    Signed,
    Float,
    SFloat,
    Double,
    Unorm,
    Count
  };

  enum class FormatType
  {
    Unknown = 0,
    Double,
    Uint32RGBA,
    Uint32RGB,
    Uint32RG,
    Uint32,
    Int32,
    Float32RGBA,
    Float32RGB,
    Float32RG,
    Float32,
    Float16RGBA,
    Float16RG,
    Float16,
    Unorm16RGBA,
    Unorm16RG,
    Unorm16,
    Uint16RGBA,
    Uint16RG,
    Uint16,
    Int16,
    Uint8RGBA,
    Uint8RG,
    Int8RGBA,
    Unorm8RGBA,
    Unorm8SRGBA,
    Unorm8BGRA,
    Unorm8SBGRA,
    Unorm10RGB2A,
    Raw32,
    Depth32,
    Depth24,
    Depth32_Stencil8,
    Int8,
    Uint8,
    Unorm8,
    Count
  };

  struct FormatSizeInfo
  {
    FormatType fm;
    int pixelSize;
    int componentCount;
    FormatTypeIdentifier pixelType;
  };

  FormatType findFormat(int components, int bits, FormatTypeIdentifier pixelType);
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