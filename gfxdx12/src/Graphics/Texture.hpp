#pragma once
#include "Descriptors/ResUsage.hpp"
#include "FazCPtr.hpp"
#include "RawResource.hpp"
#include "ResourceDescriptor.hpp"
#include "core/src/memory/ManagedResource.hpp"

#include <d3d12.h>
#include <memory>

//////////////////////////////////////////////////////////////////
// New stuff

template<typename type>
struct MappedTexture
{
  std::shared_ptr<RawResource> m_mappedresource;
  D3D12_RANGE range;
  bool readback;
  type* mapped;

  MappedTexture(type* ptr)
    : m_mappedresource(nullptr)
    , range({})
    , readback(false)
    , mapped(ptr)
  {}

  MappedTexture(std::shared_ptr<RawResource> res, D3D12_RANGE r, bool readback, type* ptr)
    : m_mappedresource(res)
    , range(r)
    , readback(readback)
    , mapped(ptr)
  {}

  ~MappedTexture()
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

struct TextureInternal
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
      return MappedTexture<T>(nullptr);
    }
    T* ptr;
    m_range.Begin = 0;
    m_range.End = width*height*stride; // TODO: !??!?!??!
    HRESULT hr = m_resource->Map(0, &m_range, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
      return MappedTexture<T>(nullptr);
    }
    return MappedTexture<T>(m_resource, m_range, (m_desc.m_usage == ResourceUsage::ReadbackHeap), ptr);
  }

  bool isValid()
  {
	  return m_resource.get() != nullptr;
  }
};

// public interace?
class Texture
{
  friend class GpuDevice;
  std::shared_ptr<TextureInternal> texture;
public:

  template<typename T>
  MappedTexture<T> Map()
  {
    return buffer->Map<T>();
  }

  TextureInternal& getTexture()
  {
    return *texture;
  }

  bool isValid()
  {
    return texture->isValid();
  }
};

class TextureShaderView
{
private:
  friend class GpuDevice;
  friend class GraphicsCmdBuffer;
  Texture m_texture; // keep texture alive here, if copying is issue like it could be. TODO: REFACTOR
  ShaderViewDescriptor viewDesc;
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  FazPtr<size_t> indexInHeap; // will handle removing references from heaps when destructed. ref counted.
  size_t customIndex;
public:
  TextureInternal& texture()
  {
    return m_texture.getTexture();
  }

  bool isValid()
  {
    return m_texture.getTexture().isValid();
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

class TextureSRV : public TextureShaderView
{

};

class TextureUAV : public TextureShaderView
{

};

class TextureRTV : public TextureShaderView
{

};

class TextureDSV : public TextureShaderView
{

};
