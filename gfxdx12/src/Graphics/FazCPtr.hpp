#pragma once
#include <wrl.h>
#include <atomic>
#include "core/src/global_debug.hpp"

static std::atomic<int> s_count;

// TODO: overhaul this to work, somehow maybe leaking stuff?
// https://github.com/Microsoft/DirectXTK/wiki/ComPtr

template <typename T>
using FazCPtr = Microsoft::WRL::ComPtr<T>;

/*
template <typename T>
class FazCPtr
{
private:
  Microsoft::WRL::ComPtr<T> ptr;
public:
  FazCPtr() : ptr(nullptr)
  {
	  s_count++;
	  F_LOG("created, %d left\n", s_count);
  }
  FazCPtr(T* type) :ptr(Microsoft::WRL::ComPtr<T>(type))
  {
	  s_count++;
	  F_LOG("created, %d left\n", s_count);
  }

  FazCPtr(FazCPtr&& other)
	  : ptr(std::move(other.ptr))
  {
	  s_count++;
	  F_LOG("created, %d left\n", s_count);
  }

  FazCPtr(const FazCPtr& other)
	  : ptr(other.ptr)
  {
	  s_count++;
	  F_LOG("created, %d left\n", s_count);
  }

  FazCPtr& operator=(const FazCPtr& other)
  {
	  ptr = other.ptr;
	  return *this;
  }

  FazCPtr& operator=(FazCPtr&& other)
  {
	  ptr = std::move(other.ptr);
	  return *this;
  }

  ~FazCPtr()
  {
	  s_count--;
	  F_LOG("Released, %d left\n", s_count);
  }

  T* operator->() const throw()
  {
    return ptr.operator->();
  }

  T* get() const
  {
    return ptr.Get();
  }

  T** addr()
  {
    return ptr.GetAddressOf();
  }

  T** ReleaseAndGetAddressOf()
  {
	  return ptr.ReleaseAndGetAddressOf();
  }
};

*/