#include "GpuDevice.hpp"

GpuDevice::GpuDevice(GpuDeviceImpl device)
  : m_device(device)
{
}

GraphicsQueue GpuDevice::createGraphicsQueue()
{
  return m_device.createGraphicsQueue();
}

DMAQueue GpuDevice::createDMAQueue()
{
  return m_device.createDMAQueue();
}
ComputeQueue GpuDevice::createComputeQueue()
{
  return m_device.createComputeQueue();
}

DMACmdBuffer GpuDevice::createDMACommandBuffer()
{
  return m_device.createDMACommandBuffer();
}

GraphicsPipeline GpuDevice::createGraphicsPipeline(GraphicsPipelineDescriptor desc)
{
  return m_device.createGraphicsPipeline(desc);
}
/*
ComputePipeline GpuDevice::createComputePipeline(ComputePipelineDescriptor desc)
{
  return m_device.createComputePipeline(desc);
}*/

ComputeCmdBuffer GpuDevice::createComputeCommandBuffer()
{
  return m_device.createComputeCommandBuffer();
}

GraphicsCmdBuffer GpuDevice::createGraphicsCommandBuffer()
{
  return m_device.createGraphicsCommandBuffer();
}

ResourceHeap GpuDevice::createMemoryHeap(HeapDescriptor desc)
{
  F_ASSERT(desc.m_sizeInBytes != 0, "Not valid to create memory heap of size 0");

  auto ensureAlignment = [](uint64_t sizeInBytes, uint64_t alignment)
  {
    uint64_t spill = sizeInBytes % alignment;
    if (spill == 0)
    {
      return sizeInBytes;
    }
    return sizeInBytes + alignment - spill;
  };

  desc.m_sizeInBytes = ensureAlignment(desc.m_sizeInBytes, desc.m_alignment);
  F_ASSERT(desc.m_sizeInBytes < ResourceHeap::s_pageCount * desc.m_alignment, "Trying to create heap that is too large for the heap page count, count was %u < %u", desc.m_sizeInBytes, ResourceHeap::s_pageCount * desc.m_alignment);
  return m_device.createMemoryHeap(desc);
}

Buffer GpuDevice::createBuffer(ResourceHeap& heap, ResourceDescriptor desc)
{
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Default && desc.m_usage != ResourceUsage::GpuOnly), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Readback && desc.m_usage != ResourceUsage::ReadbackHeap), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Upload && desc.m_usage != ResourceUsage::UploadHeap), "");
  return m_device.createBuffer(heap, desc);
}
Texture GpuDevice::createTexture(ResourceHeap& heap, ResourceDescriptor desc)
{
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Default && desc.m_usage != ResourceUsage::GpuOnly), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Readback && desc.m_usage != ResourceUsage::ReadbackHeap), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Upload && desc.m_usage != ResourceUsage::UploadHeap), "");
  return m_device.createTexture(heap, desc);
}
// shader views
TextureSRV GpuDevice::createTextureSRV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureSRV view;
  view.m_texture = targetTexture;
  view.m_view = m_device.createTextureView(targetTexture.getTexture(), viewDesc);
  return view;
}
TextureUAV GpuDevice::createTextureUAV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureUAV view;
  view.m_texture = targetTexture;
  view.m_view = m_device.createTextureView(targetTexture.getTexture(), viewDesc);
  return view;
}
TextureRTV GpuDevice::createTextureRTV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureRTV view;
  view.m_texture = targetTexture;
  view.m_view = m_device.createTextureView(targetTexture.getTexture(), viewDesc);
  return view;
}
TextureDSV GpuDevice::createTextureDSV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureDSV view;
  view.m_texture = targetTexture;
  view.m_view = m_device.createTextureView(targetTexture.getTexture(), viewDesc);
  return view;
}

BufferSRV GpuDevice::createBufferSRV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferSRV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device.createBufferView(targetBuffer.getBuffer(), viewDesc);
  return view;
}
BufferUAV GpuDevice::createBufferUAV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferUAV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device.createBufferView(targetBuffer.getBuffer(), viewDesc);
  return view;
}
BufferCBV GpuDevice::createBufferCBV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferCBV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device.createBufferView(targetBuffer.getBuffer(), viewDesc);
  return view;
}
BufferIBV GpuDevice::createBufferIBV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferIBV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device.createBufferView(targetBuffer.getBuffer(), viewDesc);
  return view;
}

bool GpuDevice::isValid()
{
  return m_device.isValid();
}