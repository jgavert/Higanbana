#pragma once
#include "Descriptors/ResType.hpp"
#include "ComPtr.hpp"
#include <d3d12.h>

template<typename type>
struct MappedBuffer
{
  ComPtr<ID3D12Resource> m_mappedresource;
  D3D12_RANGE range;
  type* mapped;
  MappedBuffer(ComPtr<ID3D12Resource> res, D3D12_RANGE r, type* ptr)
    : m_mappedresource(res)
    , range(r)
    , mapped(ptr)
  {}
  ~MappedBuffer()
  {
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

  type* get()
  {
    return mapped;
  }
};

struct Buffer
{
  ComPtr<ID3D12Resource> m_resource;
  size_t size;
  size_t stride;
  D3D12_RESOURCE_STATES state;
  D3D12_RANGE range;
  ResType type;

  template<typename T>
  MappedBuffer<T> Map()
  {
    if (type == ResType::Gpu)
    {
      // insert assert
    }
    T* ptr;
    range.Begin = 0;
    range.End = size*stride;
    HRESULT hr = m_resource->Map(0, &range, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
      // something?
    }
    return MappedBuffer<T>(m_resource, range, ptr);
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
    return m_buffer.m_resource.get() != nullptr;
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
