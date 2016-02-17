#pragma once
#include "Descriptors/ResUsage.hpp"
#include "ResourceDescriptor.hpp"
#include "ComPtr.hpp"
#include <d3d12.h>
#include <vector>

template<typename type>
struct MappedBuffer
{
  ComPtr<ID3D12Resource> m_mappedresource;
  D3D12_RANGE range;
  bool readback;
  type* mapped;
  MappedBuffer(ComPtr<ID3D12Resource> res, D3D12_RANGE r, bool readback, type* ptr)
    : m_mappedresource(res)
    , range(r)
    , mapped(ptr)
    , readback(readback)
  {}
  ~MappedBuffer()
  {
    if (readback)
      range.End = 0;
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

struct BufferView
{
private:
  friend class GpuDevice;
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  size_t index;
public:
  D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle()
  {
    return cpuHandle;
  }
  D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle()
  {
    return gpuHandle;
  }
};

struct Buffer
{
  ComPtr<ID3D12Resource> m_resource;
  size_t size;
  size_t stride;
  int shader_heap_index;
  D3D12_RESOURCE_STATES state;
  D3D12_RANGE range;
  ResUsage type;
  BufferView view;

  template<typename T>
  MappedBuffer<T> Map()
  {
    if (type == ResUsage::Gpu)
    {
      // insert assert
      //return nullptr;
    }
    T* ptr;
    range.Begin = 0;
    range.End = size*stride;
    HRESULT hr = m_resource->Map(0, &range, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
      // something?
		abort();
      //return nullptr;
    }
    return MappedBuffer<T>(m_resource, range, (type == ResUsage::Readback), ptr);
  }

  bool isValid()
  {
    return m_resource.get() != nullptr;
  }
};

class _Buffer
{
private:

  Buffer m_buffer;
public:
  Buffer& buffer() { return m_buffer; }

  bool isValid()
  {
    return m_buffer.isValid();
  }
};

class BufferSRV : public _Buffer
{
public:
 
};

class BufferUAV : public _Buffer
{
public:

};

class BufferIBV : public _Buffer
{
public:

};

class BufferCBV : public _Buffer
{
public:

};

//////////////////////////////////////////////////////////////////
// New stuff

struct Buffer_new
{
  ComPtr<ID3D12Resource> m_resource; 
  ResourceDescriptor m_desc;
  D3D12_RESOURCE_STATES m_state; // important and all shader views should share this
  bool m_immutableState;
  D3D12_RANGE m_range;

  template<typename T>
  MappedBuffer<T> Map()
  {
    if (type == ResourceUsage::Gpu)
    {
      abort();
    }
    T* ptr;
    m_range.Begin = 0;
    m_range.End = size*stride;
    HRESULT hr = m_resource->Map(0, &m_range, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
      // something?
      abort();
    }
    return MappedBuffer<T>(m_resource, m_range, (m_desc.m_format == ResourceUsage::Readback), ptr);
  }

  bool isValid()
  {
    return m_resource.get() != nullptr;
  }
};