#pragma once
#include "viewport.hpp"
#include "Fence.hpp"
#include "Buffer.hpp"
#include "ComPtr.hpp"
#include "binding.hpp"
#include "Pipeline.hpp"
#include "core/src/math/vec_templated.hpp"

#include <d3d12.h>
// universal command buffer

class CptCommandList
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuCommandQueue;
  friend class GpuDevice;
  friend class GfxCommandList;

  ComPtr<ID3D12GraphicsCommandList> m_CommandList;
  CptCommandList(ComPtr<ID3D12GraphicsCommandList> cmdList);
public:
  CptCommandList() : m_CommandList(nullptr) {}
  void setViewPort(ViewPort view);
  void ClearRenderTargetView(TextureRTV rtv, faze::vec4 color);
  void CopyResource(Buffer& dstdata, Buffer& srcdata);
  void setResourceBarrier();
  void Dispatch(ComputeBinding bind, unsigned int x, unsigned int y, unsigned int z);
  void DispatchIndirect(ComputeBinding bind);
  ComputeBinding bind(ComputePipeline& pipeline);
  bool isValid();
};

class GfxCommandList : public CptCommandList
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuCommandQueue;
  friend class GpuDevice;
  GfxCommandList(ComPtr<ID3D12GraphicsCommandList> cmdList) :CptCommandList(cmdList){}
public:
  GfxCommandList() : CptCommandList() {}
  GraphicsBinding bind(GraphicsPipeline& pipeline);
  ComputeBinding bind(ComputePipeline& pipeline)
  {
    return CptCommandList::bind(pipeline);
  }
};
