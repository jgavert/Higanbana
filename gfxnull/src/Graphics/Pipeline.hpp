#pragma once
#include "binding.hpp"
#include "ShaderInterface.hpp"
#include "GpuResourceViewHeaper.hpp"
#include <utility>

class ComputePipeline
{
	friend class GpuDevice;

  void* pipeline;
  ShaderInterface m_shaderInterface;
  ComputeBinding rootBinding;
  ResourceViewManager m_heap;

public:
  ComputePipeline() :
    pipeline(nullptr),
    m_shaderInterface() {}
  ComputeBinding getBinding()
  {
    return rootBinding;
  }
  void* getState()
  {
    return pipeline;
  }
  ShaderInterface& getShaderInterface()
  {
    return m_shaderInterface;
  }
  ResourceViewManager& getDescHeap()
  {
	  return m_heap;
  }
  bool valid()
  {
    return true;
  }
};

class GraphicsPipeline
{
	friend class GpuDevice;

  void* pipeline;
  ShaderInterface m_shaderInterface;
  GraphicsBinding rootDescriptor;
  ResourceViewManager m_heap;

public:
  GraphicsPipeline() :
    pipeline(nullptr),
    m_shaderInterface() {}
  GraphicsBinding getBinding()
  {
    return rootDescriptor;
  }
  void* getState()
  {
    return pipeline;
  }
  ShaderInterface& getShaderInterface()
  {
    return m_shaderInterface;
  }
  ResourceViewManager& getDescHeap()
  {
	  return m_heap;
  }
  bool valid()
  {
    return true;
  }
};
