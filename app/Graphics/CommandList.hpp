#pragma once
#include "viewport.hpp"
#include "Fence.hpp"
#include "Buffer.hpp"
#include "ComPtr.hpp"
#include "binding.hpp"
#include "Pipeline.hpp"
#include "core/src/math/vec_templated.hpp"
#include "ShaderInterface.hpp"
#include "GpuResourceViewHeaper.hpp"
#include "DescriptorHeapManager.hpp"

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
  friend class _GpuBracket;

  ComPtr<ID3D12GraphicsCommandList> m_CommandList;
  ComPtr<ID3D12CommandAllocator> m_CommandListAllocator;
  bool closed;
  ShaderInterface m_boundShaderInterface;
  ComputePipeline* m_boundCptPipeline;
  GraphicsPipeline* m_boundGfxPipeline;

  int m_srvBindlessIndex;
  int m_uavBindlessIndex;

  CptCommandList(ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3D12CommandAllocator> commandListAllocator);
public:
  CptCommandList() : m_CommandList(nullptr), closed(false), m_boundCptPipeline(nullptr), m_boundGfxPipeline(nullptr),
    m_uavBindlessIndex(-1), m_srvBindlessIndex(-1){}

  void profilingEventStart(std::string name)
  {
    // cannot be implemented yet, missing d3d12 side of things.
  }
  void CopyResource(Buffer& dstdata, Buffer& srcdata);
  void setResourceBarrier();
  void Dispatch(ComputeBinding bind, unsigned int x, unsigned int y, unsigned int z);
  void DispatchIndirect(ComputeBinding bind);
  ComputeBinding bind(ComputePipeline& pipeline);

  void setHeaps(DescriptorHeapManager& heaps)
  {
    m_CommandList->SetDescriptorHeaps(heaps.getCount(), heaps.getHeapPtr());
  }

  void setSRVBindless(DescriptorHeapManager& srvDescHeap)
  {
    if (m_srvBindlessIndex < 0)
      return;
    auto srvGpuStart = srvDescHeap.getGeneric().m_descHeap->GetGPUDescriptorHandleForHeapStart();
    srvGpuStart.ptr += srvDescHeap.getGeneric().m_handleIncrementSize * 128;
    m_CommandList->SetComputeRootDescriptorTable(m_srvBindlessIndex, srvGpuStart);
  }
  void setUAVBindless(DescriptorHeapManager& uavDescHeap)
  {
    if (m_uavBindlessIndex < 0)
      return;
    auto uavGpuStart = uavDescHeap.getGeneric().m_descHeap->GetGPUDescriptorHandleForHeapStart();
    uavGpuStart.ptr += uavDescHeap.getGeneric().m_handleIncrementSize * 128 * 2;
    m_CommandList->SetComputeRootDescriptorTable(m_uavBindlessIndex, uavGpuStart);
  }
  bool isValid();
  void close()
  {
    HRESULT hr;
    hr = m_CommandList->Close();
    if (FAILED(hr))
    {
      //
    }
    closed = true;
  }
  bool isClosed()
  {
    return closed;
  }

  void resetList()
  {
    m_CommandListAllocator->Reset();
    if (m_boundCptPipeline != nullptr)
      m_CommandList->Reset(m_CommandListAllocator.get(), m_boundCptPipeline->getState());
    else
      m_CommandList->Reset(m_CommandListAllocator.get(), nullptr);
    closed = false;
    if (m_boundShaderInterface.valid())
      m_CommandList->SetComputeRootSignature(m_boundShaderInterface.m_rootSig.get());
  }
};

class GfxCommandList : public CptCommandList
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuCommandQueue;
  friend class GpuDevice;
  friend class _GpuBracket;
  GfxCommandList(ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3D12CommandAllocator> commandListAllocator) :CptCommandList(cmdList, commandListAllocator){}
public:
  GfxCommandList() : CptCommandList() {}
  GraphicsBinding bind(GraphicsPipeline& pipeline);
  ComputeBinding bind(ComputePipeline& pipeline)
  {
    return CptCommandList::bind(pipeline);
  }

  void setViewPort(ViewPort view);
  void ClearRenderTargetView(TextureRTV rtv, faze::vec4 color);
  void ClearDepthStencilView(TextureDSV dsv);
  void ClearStencilView(TextureDSV dsv);
  void ClearDepthView(TextureDSV dsv);
  void drawInstanced(GraphicsBinding bind, unsigned int vertexCountPerInstance, unsigned int instanceCount, unsigned int startVertexId, unsigned int startInstanceId)
  {
    if (bind.m_resbars.size() > 0)
    {
      m_CommandList->ResourceBarrier(static_cast<UINT>(bind.m_resbars.size()), bind.m_resbars.data());
      bind.m_resbars.clear();
    }
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
    m_CommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexId, startInstanceId);
  }

  void drawInstancedRaw(unsigned int vertexCountPerInstance, unsigned int instanceCount, unsigned int startVertexId, unsigned int startInstanceId)
  {
    m_CommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexId, startInstanceId);
  }

  void setRenderTarget(TextureRTV rtv)
  {
    m_CommandList->OMSetRenderTargets(1, &rtv.texture().view.getCpuHandle(), false, nullptr);
  }

  void setRenderTarget(TextureRTV rtv, TextureDSV dsv)
  {
    m_CommandList->OMSetRenderTargets(1, &rtv.texture().view.getCpuHandle(), false, &dsv.texture().view.getCpuHandle());
  }

  void setSRVBindless(DescriptorHeapManager& srvDescHeap)
  {
    if (m_srvBindlessIndex < 0)
      return;
    auto srvGpuStart = srvDescHeap.getGeneric().m_descHeap->GetGPUDescriptorHandleForHeapStart();
    srvGpuStart.ptr += srvDescHeap.getGeneric().m_handleIncrementSize * 128;
    m_CommandList->SetGraphicsRootDescriptorTable(m_srvBindlessIndex, srvGpuStart);
  }
  void setUAVBindless(DescriptorHeapManager& uavDescHeap)
  {
    if (m_uavBindlessIndex < 0)
      return;
    auto uavGpuStart = uavDescHeap.getGeneric().m_descHeap->GetGPUDescriptorHandleForHeapStart();
    uavGpuStart.ptr += uavDescHeap.getGeneric().m_handleIncrementSize * 128 * 2;
    m_CommandList->SetGraphicsRootDescriptorTable(m_uavBindlessIndex, uavGpuStart);
  }

  void resetList()
  {
    m_CommandListAllocator->Reset();
    m_CommandList->Reset(m_CommandListAllocator.get(), nullptr);
    closed = false;
    m_boundCptPipeline = nullptr;
    m_boundGfxPipeline = nullptr;
    m_boundShaderInterface = ShaderInterface();
  }
};
