#pragma once
#include "Descriptors/ResUsage.hpp"
#include "ComPtr.hpp"
#include "RawResource.hpp"
#include "ResourceDescriptor.hpp"
#include "core/src/memory/ManagedResource.hpp"

#include <d3d12.h>
#include <memory>

template<typename type>
struct MappedTexture // TODO: add support for "invalid mapping".
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

//////////////////////////////////////////////////////////////////
// New stuff

template<typename type>
struct MappedTexture_new
{
  std::shared_ptr<RawResource> m_mappedresource;
  D3D12_RANGE range;
  bool readback;
  type* mapped;

  MappedTexture_new(type* ptr)
    : m_mappedresource(nullptr)
    , range({})
    , readback(false)
    , mapped(ptr)
  {}

  MappedTexture_new(std::shared_ptr<RawResource> res, D3D12_RANGE r, bool readback, type* ptr)
    : m_mappedresource(res)
    , range(r)
    , readback(readback)
    , mapped(ptr)
  {}

  ~MappedTexture_new()
  {
    if (valid())
    {
      if (readback)
      {
        range.End = 0;
      }
      m_mappedresource->Unmap(0, &range);
    }
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

  // should always check if mapping is valid.
  bool valid()
  {
    return mapped != nullptr;
  }
};

struct Texture_new
{
  std::shared_ptr<RawResource> m_resource; // SpecialWrapping so that we can take "not owned" objects as our resources.
  ResourceDescriptor m_desc;
  D3D12_RESOURCE_STATES m_state;
  bool m_immutableState;
  D3D12_RANGE m_range;

  template<typename T>
  MappedTexture<T> Map()
  {
    if (m_desc.m_usage == ResourceUsage::Gpu)
    {
      return MappedTexture_new<T>(nullptr);
    }
    T* ptr;
    m_range.Begin = 0;
    m_range.End = width*height*stride; // TODO: !??!?!??!
    HRESULT hr = m_resource->Map(0, &m_range, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
      return MappedTexture_new<T>(nullptr);
    }
    return MappedTexture_new<T>(m_resource, m_range, (m_desc.m_usage == ResourceUsage::ReadbackHeap), ptr);
  }

  bool isValid()
  {
	  return m_resource.get() != nullptr;
  }
};

class TextureShaderView
{
private:
  friend class GpuDevice;
  Texture_new m_texture; // keep texture alive here, if copying is issue like it could be. TODO: REFACTOR
  ShaderViewDescriptor viewDesc;
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  FazPtr<size_t> indexInHeap; // will handle removing references from heaps when destructed. ref counted.
  size_t customIndex;
public:
  Texture_new& texture()
  {
    return m_texture;
  }

  bool isValid()
  {
    return m_texture.m_resource.get() != nullptr;
  }

  size_t getIndexInHeap()
  {
    return *indexInHeap.get(); // This is really confusing getter, for completely wrong reasons.
  }

  unsigned getCustomIndexInHeap() // this returns implementation specific index. There might be better ways to do this.
  {
    return static_cast<unsigned>(customIndex);
  }
};

// to separate different views typewise, also enables information to know from which view we are trying to move into.

class TextureNewSRV : public TextureShaderView
{

};

class TextureNewUAV : public TextureShaderView
{

};

class TextureNewRTV : public TextureShaderView
{

};

class TextureNewDSV : public TextureShaderView
{

};
