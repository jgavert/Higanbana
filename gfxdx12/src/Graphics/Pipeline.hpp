#pragma once
#include "FazCPtr.hpp"
#include "binding.hpp"
#include "ShaderInterface.hpp"
#include "GpuResourceViewHeaper.hpp"
#include <d3d12.h>
#include <utility>

class ComputePipeline
{
	friend class GpuDevice;

  FazCPtr<ID3D12PipelineState> pipeline;
  ShaderInterface m_shaderInterface;
  ComputeBinding rootBinding;
  ResourceViewManager m_heap;


  ComputePipeline(FazCPtr<ID3D12PipelineState> pipe, ShaderInterface shaderInterface, ComputeBinding sourceBinding, ResourceViewManager heap):
    pipeline(std::move(pipe)),
    m_shaderInterface(shaderInterface),
    rootBinding(sourceBinding),
	m_heap(heap) {}
public:
  ComputePipeline() :
    pipeline(nullptr),
    m_shaderInterface() {}
  ComputeBinding getBinding()
  {
    return rootBinding;
  }
  ID3D12PipelineState* getState()
  {
    return pipeline.get();
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
    return pipeline.get() != nullptr;
  }
};

class GraphicsPipeline
{
	friend class GpuDevice;

  FazCPtr<ID3D12PipelineState> pipeline;
  ShaderInterface m_shaderInterface;
  GraphicsBinding rootDescriptor;
  ResourceViewManager m_heap;

  GraphicsPipeline(FazCPtr<ID3D12PipelineState> pipe, ShaderInterface shaderInterface, GraphicsBinding sourceBinding, ResourceViewManager heap):
    pipeline(std::move(pipe)),
    m_shaderInterface(shaderInterface),
    rootDescriptor(sourceBinding),
	m_heap(heap) {}
public:
  GraphicsPipeline() :
    pipeline(nullptr),
    m_shaderInterface() {}
  GraphicsBinding getBinding()
  {
    return rootDescriptor;
  }
  ID3D12PipelineState* getState()
  {
    return pipeline.get();
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
    return pipeline.get() != nullptr;
  }
};
