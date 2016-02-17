#pragma once
#include <wrl.h>

template <typename T>
class ComPtr
{
private:
  Microsoft::WRL::ComPtr<T> ptr;
public:
  ComPtr() : ptr(nullptr){}
  ComPtr(T* type) :ptr(type) {}

  T* operator->() const throw()
  {
    return ptr.operator->();
  }

  T* get()
  {
    return ptr.Get();
  }

  T** addr()
  {
    return ptr.GetAddressOf();
  }
};
