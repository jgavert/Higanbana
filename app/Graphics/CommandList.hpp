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
  CptCommandList() :
	  m_CommandList(nullptr),
	  closed(false),
	  m_boundCptPipeline(nullptr),
	  m_boundGfxPipeline(nullptr),
	  m_uavBindlessIndex(-1),
	  m_srvBindlessIndex(-1)
  {}
  void CopyResource(Buffer& dstdata, Buffer& srcdata);
  void setResourceBarrier();
  void bindComputeBinding(ComputeBinding& bind);
  void Dispatch(ComputeBinding& bind, unsigned int x, unsigned int y, unsigned int z);
  void DispatchIndirect(ComputeBinding& bind);
  ComputeBinding bind(ComputePipeline& pipeline);
  void setHeaps(DescriptorHeapManager& heaps);
  void setSRVBindless(DescriptorHeapManager& srvDescHeap);
  void setUAVBindless(DescriptorHeapManager& uavDescHeap);
  bool isValid();
  void close();
  bool isClosed();
  void resetList();
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
  ComputeBinding bind(ComputePipeline& pipeline);
  void setViewPort(ViewPort& view);
  void ClearRenderTargetView(TextureRTV& rtv, faze::vec4 color);
  void ClearDepthStencilView(TextureDSV& dsv);
  void ClearStencilView(TextureDSV& dsv);
  void ClearDepthView(TextureDSV& dsv);
  void bindGraphicsBinding(ComputeBinding& bind);
  void drawInstanced(GraphicsBinding& bind, unsigned int vertexCountPerInstance, unsigned int instanceCount, unsigned int startVertexId, unsigned int startInstanceId);
  void drawInstancedRaw(unsigned int vertexCountPerInstance, unsigned int instanceCount, unsigned int startVertexId, unsigned int startInstanceId);
  void setRenderTarget(TextureRTV& rtv);
  void setRenderTarget(TextureRTV& rtv, TextureDSV& dsv);
  void setSRVBindless(DescriptorHeapManager& srvDescHeap);
  void setUAVBindless(DescriptorHeapManager& uavDescHeap);
  void resetList();
};
