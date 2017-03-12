#pragma once

// This should be wrapped with shared_ptr
// You wan't to avoid calling the destructor
struct RawResource
{
  void* p_resource;
  
  bool m_owned;

  RawResource(void* ptr, bool owned = true) // usually owned
    : p_resource(ptr)
    , m_owned(owned)
  {}

  ~RawResource()
  {
  }

  void* operator->() const throw()
  {
    return p_resource;
  }

  void* get()
  {
    return p_resource;
  }
};