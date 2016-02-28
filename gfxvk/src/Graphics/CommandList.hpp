#pragma once
#include "viewport.hpp"
#include "Fence.hpp"
#include "Buffer.hpp"
#include "binding.hpp"
#include "Pipeline.hpp"
#include "core/src/math/vec_templated.hpp"
#include "ShaderInterface.hpp"
#include "GpuResourceViewHeaper.hpp"
#include "DescriptorHeapManager.hpp"

// universal command buffer

class CptCommandList
{
private:
  friend class GpuCommandQueue;
  friend class GpuDevice;
  friend class GfxCommandList;
  friend class _GpuBracket;

  void* m_CommandList;
  bool closed;

  CptCommandList(void* cmdList);
public:
  CptCommandList() :
	  m_CommandList(nullptr),
	  closed(false)
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
  void closeList();
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
  GfxCommandList(void* cmdList) :CptCommandList(cmdList){}
public:
  GfxCommandList() : CptCommandList() {}
  GraphicsBinding bind(GraphicsPipeline& pipeline);
  ComputeBinding bind(ComputePipeline& pipeline);
  void setViewPort(ViewPort& view);
  void ClearRenderTargetView(TextureRTV& rtv, faze::vec4 color);
  void ClearDepthStencilView(TextureDSV& dsv);
  void ClearStencilView(TextureDSV& dsv);
  void ClearDepthView(TextureDSV& dsv);
  void bindGraphicsBinding(GraphicsBinding& bind);
  void drawInstanced(GraphicsBinding& bind, unsigned int vertexCountPerInstance, unsigned int instanceCount = 1, unsigned int startVertexId = 0, unsigned int startInstanceId = 0);
  void drawInstancedRaw(unsigned int vertexCountPerInstance, unsigned int instanceCount, unsigned int startVertexId, unsigned int startInstanceId);
  void setRenderTarget(TextureRTV& rtv);
  void setRenderTarget(TextureRTV& rtv, TextureDSV& dsv);
  void preparePresent(TextureRTV& rtv);
  void setSRVBindless(DescriptorHeapManager& srvDescHeap);
  void setUAVBindless(DescriptorHeapManager& uavDescHeap);
  void resetList();
};
