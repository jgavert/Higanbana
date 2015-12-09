#pragma once
#include "ComPtr.hpp"
#include "binding.hpp"
#include "ShaderInterface.hpp"
#include <d3d12.h>
#include <utility>

class ComputePipeline
{
	friend class GpuDevice;

  ComPtr<ID3D12PipelineState> pipeline;
  ShaderInterface m_shaderInterface;
  ComputeBinding rootBinding;

  ComputePipeline(ComPtr<ID3D12PipelineState> pipe, ShaderInterface shaderInterface, ComputeBinding sourceBinding):
    pipeline(std::move(pipe)),
    m_shaderInterface(shaderInterface),
    rootBinding(sourceBinding) {}
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
  bool valid()
  {
    return pipeline.get() != nullptr;
  }
};

class GraphicsPipeline
{
	friend class GpuDevice;

  ComPtr<ID3D12PipelineState> pipeline;
  ShaderInterface m_shaderInterface;
  GraphicsBinding rootDescriptor;
  GraphicsPipeline(ComPtr<ID3D12PipelineState> pipe, ShaderInterface shaderInterface, GraphicsBinding sourceBinding):
    pipeline(std::move(pipe)),
    m_shaderInterface(shaderInterface),
    rootDescriptor(sourceBinding) {}
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
  bool valid()
  {
    return pipeline.get() != nullptr;
  }
};
