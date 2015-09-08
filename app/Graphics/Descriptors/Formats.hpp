#pragma once
#include <d3d12.h>

// declare all formats and their d3d12 counterparts
// all tables must be kepts insync

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

static DXGI_FORMAT FormatToDXGIFormat[] =
{
  DXGI_FORMAT_UNKNOWN,
  DXGI_FORMAT_R32G32B32A32_TYPELESS,
  DXGI_FORMAT_R32G32B32A32_FLOAT,
  DXGI_FORMAT_R32G32B32_TYPELESS,
  DXGI_FORMAT_R32G32B32_FLOAT,
  DXGI_FORMAT_R8G8B8A8_TYPELESS,
  DXGI_FORMAT_R8G8B8A8_UNORM,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
  DXGI_FORMAT_R32_TYPELESS,
  DXGI_FORMAT_D32_FLOAT,
  DXGI_FORMAT_R8_TYPELESS,
  DXGI_FORMAT_R8_UNORM
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
