#pragma once

#include <functional>
#include <memory>

// This should be wrapped with shared_ptr
// You wan't to avoid calling the destructor
template <typename T>
struct ManagedResource_
{
  T p_resource;
  bool m_owned;
  std::function<void(T)> destructor;
  ManagedResource_()
    :m_owned(false)
    ,destructor([](T){})
  {}
  ~ManagedResource_()
  {
    if (m_owned && p_resource)
    {
      destructor(p_resource);
    }
  }

};

// heavyweight smartpointer for resources
template <typename T>
class FazPtr
{
  std::shared_ptr<ManagedResource_<T>> resource;

public:
  FazPtr() // usually owned
  {
    resource = std::make_shared<ManagedResource_<T>>();
  }

  FazPtr(std::function<void(T)> destructor, bool owned = true) // usually owned
  {
    resource = std::make_shared<ManagedResource_<T>>();
    resource.get()->p_resource = nullptr;
    resource.get()->m_owned = owned;
    resource.get()->destructor = destructor;
  }

  FazPtr(T ptr, std::function<void(T)> destructor, bool owned = true) // usually owned
  {
    resource = std::make_shared<ManagedResource_<T>>();
    resource.get()->p_resource = ptr;
    resource.get()->m_owned = owned;
    resource.get()->destructor = destructor;
  }
  T* operator->() const throw()
  {
    return &resource->p_resource;
  }

  T* get()
  {
    return &resource->p_resource;
  }
};
