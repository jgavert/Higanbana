#include "CommandList.hpp"

#include <vulkan/vk_cpp.h>

class VulkanCmdBuffer
{
private:
  friend class GpuDevice;
  friend class VulkanQueue;
  FazPtrVk<vk::CommandBuffer>    m_cmdBuffer;
public:
  void CopyResource(Buffer& dstdata, Buffer& srcdata); // this is here only temporarily, will be removed
  void setResourceBarrier();
  void bindComputeBinding(ComputeBinding& bind);
  void Dispatch(ComputeBinding& bind, unsigned int x, unsigned int y, unsigned int z);
  void DispatchIndirect(ComputeBinding& bind);
  ComputeBinding bind(ComputePipeline& pipeline);
  void setHeaps(DescriptorHeapManager& heaps);
  void setSRVBindless(DescriptorHeapManager& srvDescHeap);
  void setUAVBindless(DescriptorHeapManager& uavDescHeap);
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
  bool isValid();
  void closeList();
  bool isClosed();
  void resetList();
  bool isValid();
};
