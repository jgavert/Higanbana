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
  Binding rootBinding;
  ComputePipeline(ComPtr<ID3D12PipelineState> pipe, ComPtr<ID3D12RootSignature> rootSig, ComPtr<ID3D12DescriptorHeap> descHeap, Binding sourceBinding):
    pipeline(std::move(pipe)),
    rootSig(std::move(rootSig)),
    descHeap(descHeap),
    rootBinding(sourceBinding) {}
public:
  Binding getBinding()
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

class GfxPipeline
{
	friend class GpuDevice;

  ComPtr<ID3D12PipelineState> pipeline;
  ComPtr<ID3D12RootSignature> rootSig;
  ComPtr<ID3D12DescriptorHeap> descHeap;
  Binding rootDescriptor;
  GfxPipeline(ComPtr<ID3D12PipelineState> pipe, ComPtr<ID3D12RootSignature> rootSig, ComPtr<ID3D12DescriptorHeap> descHeap, Binding sourceBinding):
    pipeline(std::move(pipe)),
    rootSig(std::move(rootSig)),
    descHeap(std::move(descHeap)),
    rootDescriptor(sourceBinding) {}
public:
  Binding getBinding()
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
};
