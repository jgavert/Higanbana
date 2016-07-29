#pragma once
#include <d3d12.h>

// This should be wrapped with shared_ptr
// You wan't to avoid calling the destructor
struct RawResource
{
  ID3D12Resource* p_resource;
  bool m_owned;

  RawResource(ID3D12Resource* ptr, bool owned = true) // usually owned
    : p_resource(ptr)
    , m_owned(owned)
  {}

  ~RawResource()
  {
    if (m_owned && p_resource)
    {
      p_resource->Release();
    }
  }

  ID3D12Resource* operator->() const throw()
  {
    return p_resource;
  }

  ID3D12Resource* Get()
  {
    return p_resource;
  }
};