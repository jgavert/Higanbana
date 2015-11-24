#pragma once
#include "ComPtr.hpp"
#include "binding.hpp"
#include <d3d12.h>
#include <utility>

class ComputePipeline
{
	friend class GpuDevice;

  ComPtr<ID3D12PipelineState> pipeline;
  ComPtr<ID3D12RootSignature> rootSig;
  ComPtr<ID3D12DescriptorHeap> descHeap;
  ComputeBinding rootBinding;
  ComputePipeline(ComPtr<ID3D12PipelineState> pipe, ComPtr<ID3D12RootSignature> rootSig, ComPtr<ID3D12DescriptorHeap> descHeap, ComputeBinding sourceBinding):
    pipeline(std::move(pipe)),
    rootSig(std::move(rootSig)),
    descHeap(descHeap),
    rootBinding(sourceBinding) {}
public:
  ComputeBinding getBinding()
  {
    return rootBinding;
  }
  ID3D12PipelineState* getState()
  {
    return pipeline.get();
  }
  ID3D12RootSignature* getRootSig()
  {
    return rootSig.get();
  }
  ID3D12DescriptorHeap** getDescHeap()
  {
    return descHeap.addr();
  }
};

class GraphicsPipeline
{
	friend class GpuDevice;

  ComPtr<ID3D12PipelineState> pipeline;
  ComPtr<ID3D12RootSignature> rootSig;
  ComPtr<ID3D12DescriptorHeap> descHeap;
  GraphicsBinding rootDescriptor;
  GraphicsPipeline(ComPtr<ID3D12PipelineState> pipe, ComPtr<ID3D12RootSignature> rootSig, ComPtr<ID3D12DescriptorHeap> descHeap, GraphicsBinding sourceBinding):
    pipeline(std::move(pipe)),
    rootSig(std::move(rootSig)),
    descHeap(std::move(descHeap)),
    rootDescriptor(sourceBinding) {}
public:
  GraphicsBinding getBinding()
  {
    return rootDescriptor;
  }
  ID3D12PipelineState* getState()
  {
    return pipeline.get();
  }
  ID3D12RootSignature* getRootSig()
  {
    return rootSig.get();
  }
  ID3D12DescriptorHeap* getDescHeap()
  {
    return descHeap.get();
  }
  bool valid()
  {
    return pipeline.get() != nullptr;
  }
};
