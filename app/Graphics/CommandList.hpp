#pragma once
#include "viewport.hpp"
#include "Fence.hpp"
#include "Buffer.hpp"
#include "ComPtr.hpp"
#include "binding.hpp"
#include "Pipeline.hpp"
#include <d3d12.h>
// universal command buffer

class GfxCommandList
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuCommandQueue;
  friend class GpuDevice;

  ComPtr<ID3D12GraphicsCommandList> m_CommandList;
  GfxCommandList(ComPtr<ID3D12GraphicsCommandList> cmdList);
public:
  GfxCommandList() : m_CommandList(nullptr) {}
  void setViewPort(ViewPort view);
  void CopyResource(Buffer& dstdata, Buffer& srcdata);
  void setResourceBarrier();
  void Dispatch(Binding bind, unsigned int x, unsigned int y, unsigned int z);
  void DispatchIndirect(Binding bind);
  Binding bind(GfxPipeline pipeline);
  Binding bind(ComputePipeline pipeline);
  bool isValid();
};
