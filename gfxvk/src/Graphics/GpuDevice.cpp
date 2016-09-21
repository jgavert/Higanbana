#include "GpuDevice.hpp"

GpuDevice::GpuDevice(GpuDeviceImpl device)
  : m_device(device)
  , m_queue(device.createGraphicsQueue())
{

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
/*
ComputeCmdBuffer GpuDevice::createComputeCommandBuffer()
{
  return m_device.createComputeCommandBuffer();
}*/

GraphicsCmdBuffer GpuDevice::createGraphicsCommandBuffer()
{
  return GraphicsCmdBuffer(m_device.createGraphicsCommandBuffer(), m_tracker.next());
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

void GpuDevice::submit(GraphicsCmdBuffer& gfx)
{
  LiveCmdBuffer element;
  element.cmdBuffer = gfx;
  element.fence = m_device.createFence(); // TODO: replace with ringbuffer in this device

  m_queue.submit(gfx.m_cmdBuffer, element.fence);
  m_liveCmdBuffers.emplace_back(element);
}

bool GpuDevice::fenceDone(Fence fence)
{
  if (m_tracker.hasCompleted(fence.get()))
    return true;
  updateCompletedSequences();
  return m_tracker.hasCompleted(fence.get());
}

void GpuDevice::waitFence(Fence fence)
{
  if (m_tracker.hasCompleted(fence.get()))
    return;
  while (!m_liveCmdBuffers.empty())
  {
    {
      auto&& livecmdBuffer = m_liveCmdBuffers.front();
      if (livecmdBuffer.cmdBuffer.m_seqNum == fence.get())
      {
        m_device.waitFence(livecmdBuffer.fence);
      }
    }
  }
  updateCompletedSequences();
}
void GpuDevice::waitIdle()
{
  m_device.waitIdle();
  /*
  while (!m_liveCmdBuffers.empty())
  {
    {
      auto&& livecmdBuffer = m_liveCmdBuffers.front();
      m_device.waitFence(livecmdBuffer.fence);
      m_tracker.complete(livecmdBuffer.cmdBuffer.m_seqNum);
    }
    m_liveCmdBuffers.pop_front();
  }
  */
  updateCompletedSequences();
}

void GpuDevice::updateCompletedSequences()
{
  while (!m_liveCmdBuffers.empty())
  {
    {
      auto&& livecmdBuffer = m_liveCmdBuffers.front();
      if (!m_device.checkFence(livecmdBuffer.fence))
      {
        break;
      }
      m_tracker.complete(livecmdBuffer.cmdBuffer.m_seqNum);
    }
    m_liveCmdBuffers.pop_front();
  }
}