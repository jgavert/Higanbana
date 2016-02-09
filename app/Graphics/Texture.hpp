#pragma once
#include "Descriptors/ResUsage.hpp"
#include "ComPtr.hpp"
#include <d3d12.h>

template<typename type>
struct MappedTexture
{
  ComPtr<ID3D12Resource> m_mappedresource;
  D3D12_RANGE range;
  bool readback;
  type* mapped;
  MappedTexture(ComPtr<ID3D12Resource> res, D3D12_RANGE r, bool readback, type* ptr)
    : m_mappedresource(res)
    , range(r)
    , readback(readback)
    , mapped(ptr)
  {}
  ~MappedTexture()
  {
    if (readback)
    {
      range.End = 0;
    }
    m_mappedresource->Unmap(0, &range);
  }

  size_t rangeBegin()
  {
    return range.Begin;
  }

  size_t rangeEnd()
  {
    return range.End;
  }

  type& operator[](size_t i)
  {
    return mapped[i];
  }

  type* get()
  {
    return mapped;
  }
};

struct TextureView
{
private:
  friend class GpuDevice;
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  size_t index;
public:
	TextureView()
	{
		ZeroMemory(&cpuHandle, sizeof(cpuHandle));
		ZeroMemory(&gpuHandle, sizeof(gpuHandle));
		index = 0;
	}
  D3D12_CPU_DESCRIPTOR_HANDLE& getCpuHandle()
  {
    return cpuHandle;
  }
  D3D12_GPU_DESCRIPTOR_HANDLE& getGpuHandle()
  {
    return gpuHandle;
  }
  size_t getIndex()
  {
    return index;
  }
};

struct Texture
{
  friend class GpuDevice;
  friend class GfxCommandList;
  friend class CptCommandList;
  ComPtr<ID3D12Resource> m_resource;

  size_t width;
  size_t height;
  size_t stride;
  int shader_heap_index; // TODO: Create a view and this should be inside the "view" because this is only valid in a view.
  D3D12_RESOURCE_STATES state;
  D3D12_RANGE range;
  ResUsage type;
  TextureView view;

  template<typename T>
  MappedTexture<T> Map()
  {
    if (type == ResUsage::Gpu)
    {
      // insert assert
      //return nullptr;
    }
    T* ptr;
    range.Begin = 0;
    range.End = width*height*stride;
    HRESULT hr = m_resource->Map(0, &range, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
      // something?
      //return nullptr;
    }
    return MappedTexture<T>(m_resource, range, (type == ResUsage::Readback), ptr);
  }

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
  friend class GpuDevice;
  ID3D12Resource* m_scRTV;
public:
  ID3D12Resource* textureRTV() { return m_scRTV; }
};

class TextureDSV : public _Texture
{

};
