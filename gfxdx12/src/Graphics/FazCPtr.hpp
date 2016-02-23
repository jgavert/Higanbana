#pragma once
#include <wrl.h>

// TODO: overhaul this to work, somehow maybe leaking stuff?
// https://github.com/Microsoft/DirectXTK/wiki/ComPtr
template <typename T>
class FazCPtr
{
private:
  Microsoft::WRL::ComPtr<T> ptr;
public:
  FazCPtr() : ptr(nullptr){}
  FazCPtr(T* type) :ptr(Microsoft::WRL::ComPtr<T>(type)) {}

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

  T** releaseAndAddr()
  {
	  return ptr.ReleaseAndGetAddressOf();
  }
};
