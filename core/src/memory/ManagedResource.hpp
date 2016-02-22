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
    resource.get()->p_resource = nullptr;
  }

  FazPtr(std::function<void(T)> destructor, bool owned = true) // usually owned
  {
    resource = std::make_shared<ManagedResource_<T>>();
    resource->p_resource = nullptr;
    resource->m_owned = owned;
    resource->destructor = destructor;
  }

  FazPtr(T ptr, std::function<void(T)> destructor, bool owned = true) // usually owned
  {
    resource = std::make_shared<ManagedResource_<T>>();
    resource->p_resource = ptr;
    resource->m_owned = owned;
    resource->destructor = destructor;
  }
  T* operator->() const throw()
  {
    return &resource->p_resource;
  }

  T* get()
  {
    return &resource->p_resource;
  }

  T* get() const
  {
    return &resource->p_resource;
  }

  T& getRef()
  {
    return resource->p_resource;
  }

  bool isValid()
  {
    return resource->p_resource != nullptr;
  }
};
