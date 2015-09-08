#pragma once

#include "ComPtr.hpp"
#include <d3d12.h>

struct Texture
{
  friend class GpuDevice;
  friend class GfxCommandList;
  friend class ComputeCommandList;
  ComPtr<ID3D12Resource> m_resource;
};

class _Texture
{
private:

  Texture m_texture;
public:
  Texture& texture() { return m_texture; }
  bool isValid()
  {
    return m_texture.m_resource.get() != nullptr;
  }
};

class TextureSRV : public _Texture
{

};

class TextureUAV : public _Texture
{

};

class TextureRTV : public _Texture
{

};

class TextureDSV : public _Texture
{

};
