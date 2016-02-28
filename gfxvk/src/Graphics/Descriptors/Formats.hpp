#pragma once
#include <stddef.h>

// declare all formats and their d3d12 counterparts
// all tables must be kepts insync

enum class TextureLayout
{
  Unknown,
  RowMajor,
  UndefinedSwizzle64kb,
  StandardSwizzle64kb
};

// this is more descriptive than formatbase
enum class FormatDimension
{
  Unknown,
  Buffer,
  Texture1D,
  Texture1DArray,
  Texture2D,
  Texture2DArray,
  Texture2DMS,
  Texture2DMSArray,
  Texture3D,
  TextureCube,
  TextureCubeArray
};

enum FormatDimensionBase
{
  DimUnknown,
  DimBuffer,
  DimTexture1D,
  DimTexture2D,
  DimTexture3D
};

enum FormatType
{
  Unknown = 0,
  R32G32B32A32,
  R32G32B32A32_FLOAT,
  R32G32B32,
  R32G32B32_FLOAT,
  R8G8B8A8,
  R8G8B8A8_UNORM,
  R8G8B8A8_UNORM_SRGB,
  R32,
  D32_FLOAT,
  R8,
  R8_UNORM
};

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
