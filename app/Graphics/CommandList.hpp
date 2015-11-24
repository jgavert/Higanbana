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

  void Draw(GraphicsBinding bind, unsigned int count)
  {
    if (bind.m_resbars.size() > 0)
      m_CommandList->ResourceBarrier(bind.m_resbars.size(), bind.m_resbars.data());
    for (size_t i = 0; i < bind.m_cbvs.size(); ++i)
    {
      if (bind.m_cbvs[i].first.ptr != 0)
        m_CommandList->SetGraphicsRootConstantBufferView(bind.m_cbvs[i].second, bind.m_cbvs[i].first.ptr);
    }
    for (size_t i = 0; i < bind.m_srvs.size(); ++i)
    {
      if (bind.m_srvs[i].first.ptr != 0)
        m_CommandList->SetGraphicsRootShaderResourceView(bind.m_srvs[i].second, bind.m_srvs[i].first.ptr);
    }
    for (size_t i = 0; i < bind.m_uavs.size(); ++i)
    {
      if (bind.m_uavs[i].first.ptr != 0)
        m_CommandList->SetGraphicsRootUnorderedAccessView(bind.m_uavs[i].second, bind.m_uavs[i].first.ptr);
    }
    m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->DrawInstanced(count, 1, 0, 0);
  }

  void setRenderTarget(TextureRTV rtv)
  {
    m_CommandList->OMSetRenderTargets(1, &rtv.texture().view.getCpuHandle(), false, nullptr);
  }
};
