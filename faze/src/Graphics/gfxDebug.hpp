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
  //template <typename >
  GpuBracket(T* , std::string& /*name*/)
  {
    m_bracket = std::make_shared<_ActualBracket<T>>(nullptr);
  }
  //template <typename T>
  GpuBracket(T* , const char* /*name*/)
  {
    m_bracket = std::make_shared<_ActualBracket<T>>(nullptr);
  }
};

class _GpuBracket
{
private:
public:
  static GpuBracket<void> createBracket(GraphicsQueue& , const char* name)
  {
    return GpuBracket<void>(nullptr, name);
  }
  static GpuBracket<void> createBracket(GraphicsCmdBuffer& , const char* name)
  {
    return GpuBracket<void>(nullptr, name);
  }
  static GpuBracket<void> createBracket(GraphicsQueue& , std::string& name)
  {
    return GpuBracket<void>(nullptr, name);
  }
  static GpuBracket<void> createBracket(GraphicsCmdBuffer& , std::string& name)
  {
    return GpuBracket<void>(nullptr, name);
  }
};

#if defined(FAZE_PLATFORM_WINDOWS)
#define GpuProfilingBracket(queueOrList, name) \
  auto _CONCAT(__gpuprofilingbracket, __COUNTER__) = _GpuBracket::createBracket(queueOrList, name);
#else
#define GpuProfilingBracket(queueOrList, name) \
  auto __gpuprofilingbracket##__COUNTER__ = _GpuBracket::createBracket(queueOrList, name);
#endif


