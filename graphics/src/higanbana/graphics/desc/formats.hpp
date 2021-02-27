#pragma once
#include <higanbana/core/system/helperMacros.hpp>
#include <higanbana/core/math/math.hpp>

namespace higanbana
{
  enum class ShadingRate
  {
    Rate_1x1,
    Rate_1x2,
    Rate_2x1,
    Rate_2x2,
    Rate_2x4,
    Rate_4x2,
    Rate_4x4,
    Count
  };

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
    Depth24,
    Depth24_Stencil8,
    Depth32,
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
  bool formatIsDepth(FormatType format);
  bool formatIsStencil(FormatType format);
  

  FormatType formatToSRGB(FormatType format);

  size_t sizeFormatRowPitch(int2 size, FormatType type);
  size_t sizeFormatSlicePitch(int2 size, FormatType type);
  size_t sizeFormatRowPitch(int3 size, FormatType type);
  size_t sizeFormatSlicePitch(int3 size, FormatType type);
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
    RTAccelerationStructure,
    Count
  };

  enum class LoadOp
  {
    Load,
    Clear,
    ClearArea,
    DontCare
  };

  enum class StoreOp
  {
    Store,
    DontCare
  };
}