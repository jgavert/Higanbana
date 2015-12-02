#pragma once
#include "CommandQueue.hpp"
#include "CommandList.hpp"
#include "helpers.hpp"
#include <utility>
#include <string>
#include <d3d12.h>
#include <memory>
//#define __d3d12_h__
#include <pix.h>

template <typename T>
class _ActualBracket
{
public:
  T* copyPtr;
  _ActualBracket()
    :copyPtr(nullptr)
  {
  }
  _ActualBracket(T* ptr)
    :copyPtr(ptr)
  {
  }
  ~_ActualBracket()
  {
    PIXEndEvent(copyPtr);
  }
};

template <typename T>
class GpuBracket
{
private:
  std::shared_ptr<_ActualBracket<T>> m_bracket;
public:
  template <typename T>
  GpuBracket(T* ptr, std::string& name)
  {
    PIXBeginEvent(ptr, 0, stringutils::s2ws(name).c_str());
    m_bracket = std::make_shared<_ActualBracket<T>>(ptr);
  }
  template <typename T>
  GpuBracket(T* ptr, const char* name)
  {
    PIXBeginEvent(ptr, 0, name);
    m_bracket = std::make_shared<_ActualBracket<T>>(ptr);
  }
};

class _GpuBracket
{
private:
public:
  static GpuBracket<ID3D12CommandQueue> createBracket(GpuCommandQueue& queue, const char* name)
  {
    return GpuBracket<ID3D12CommandQueue>(queue.m_CommandQueue.get(), name);
  }
  static GpuBracket<ID3D12GraphicsCommandList> createBracket(GfxCommandList& list, const char* name)
  {
    return GpuBracket<ID3D12GraphicsCommandList>(list.m_CommandList.get(), name);
  }
  static GpuBracket<ID3D12CommandQueue> createBracket(GpuCommandQueue& queue, std::string& name)
  {
    return GpuBracket<ID3D12CommandQueue>(queue.m_CommandQueue.get(), name);
  }
  static GpuBracket<ID3D12GraphicsCommandList> createBracket(GfxCommandList& list, std::string& name)
  {
    return GpuBracket<ID3D12GraphicsCommandList>(list.m_CommandList.get(), name);
  }
  /* There is no such bracket D:
  static GpuBracket<ID3D12CommandList> createBracket(CptCommandList& list, std::string& name)
  {
    return GpuBracket<ID3D12CommandList>(list.m_CommandList.get(), name);
  }*/
};

#define _CONCAT(a,b) a##b

#define GpuProfilingBracket(queueOrList, name) \
  auto _CONCAT(__gpuprofilingbracket, __COUNTER__) = _GpuBracket::createBracket(queueOrList, name);
