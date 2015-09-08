#pragma once
#include "ComPtr.hpp"
#include <d3d12.h>
#include <utility>

class ComputePipeline
{
	friend class GpuDevice;

  ComPtr<ID3D12PipelineState> pipeline;

  ComputePipeline(ComPtr<ID3D12PipelineState> pipe) : pipeline(std::move(pipe)) {}
};

class GfxPipeline
{
	friend class GpuDevice;

  ComPtr<ID3D12PipelineState> pipeline;

  GfxPipeline(ComPtr<ID3D12PipelineState> pipe) : pipeline(std::move(pipe)) {}

};
