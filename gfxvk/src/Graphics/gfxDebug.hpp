#pragma once
#include "CommandQueue.hpp"
#include "CommandList.hpp"
#include <utility>
#include <string>
#include <memory>


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
};

template <typename T>
class GpuBracket
{
private:
  std::shared_ptr<_ActualBracket<T>> m_bracket;
public:
  template <typename T>
  GpuBracket(T* ptr, std::string& /*name*/)
  {
    m_bracket = std::make_shared<_ActualBracket<T>>(ptr);
  }
  template <typename T>
  GpuBracket(T* ptr, const char* /*name*/)
  {
    m_bracket = std::make_shared<_ActualBracket<T>>(ptr);
  }
};

class _GpuBracket
{
private:
public:
  static GpuBracket<void> createBracket(GpuCommandQueue& queue, const char* name)
  {
    return GpuBracket<void>(queue.m_CommandQueue, name);
  }
  static GpuBracket<void> createBracket(GfxCommandList& list, const char* name)
  {
    return GpuBracket<void>(list.m_CommandList, name);
  }
  static GpuBracket<void> createBracket(GpuCommandQueue& queue, std::string& name)
  {
    return GpuBracket<void>(queue.m_CommandQueue, name);
  }
  static GpuBracket<void> createBracket(GfxCommandList& list, std::string& name)
  {
    return GpuBracket<void>(list.m_CommandList, name);
  }
};

#define GpuProfilingBracket(queueOrList, name) \
  auto _CONCAT(__gpuprofilingbracket, __COUNTER__) = _GpuBracket::createBracket(queueOrList, name);
