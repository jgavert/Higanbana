#include "GpuDevice.hpp"

#define COMMANDLISTCOUNT 12

GpuDevice::GpuDevice(GpuDeviceImpl device)
  : m_device(std::make_shared<GpuDeviceImpl>(device))
  , m_queue(device.createGraphicsQueue())
  , m_cmdBufferAllocator(COMMANDLISTCOUNT)
  , m_fenceAllocator(COMMANDLISTCOUNT)
{
  for (int64_t i = 0; i < COMMANDLISTCOUNT; i++)
  {
    m_rawCommandBuffers.push_back(m_device->createGraphicsCommandBuffer());
  }

  for (int64_t i = 0; i < COMMANDLISTCOUNT; i++)
  {
    m_rawFences.push_back(m_device->createFence());
  }

  for (int64_t i = 0; i < COMMANDLISTCOUNT; i++)
  {
	  m_descriptorPools.push_back(DescriptorPool(std::make_shared<DescriptorPoolImpl>(m_device->createDescriptorPool())));
  }
}

GpuDevice::~GpuDevice()
{
  waitIdle();
  updateCompletedSequences();
  destroyResources();
}

GraphicsPipeline GpuDevice::createGraphicsPipeline(GraphicsPipelineDescriptor desc)
{
  return m_device->createGraphicsPipeline(desc);
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
  auto sequence = m_tracker.next();
  auto index = m_cmdBufferAllocator.nextRange(sequence, 1);
  while (index.count() == 0)
  {
    F_ASSERT(m_liveCmdBuffers.size() != 0, "No commandbuffers live but still cannot acquire free commandbuffer.");
    updateCompletedSequences();
    index = m_cmdBufferAllocator.nextRange(sequence, 1);
  }
  auto& descPool = m_descriptorPools.at(index.start());
  auto& cmdBuffer = m_rawCommandBuffers.at(index.start());
  m_device->resetCmdBuffer(cmdBuffer);
  m_device->reset(descPool.impl());
  return GraphicsCmdBuffer(m_device, cmdBuffer, sequence, descPool);
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
  return m_device->createMemoryHeap(desc);
}

Buffer GpuDevice::createBuffer(ResourceHeap& heap, ResourceDescriptor desc)
{
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Default && desc.m_usage != ResourceUsage::GpuOnly), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Readback && desc.m_usage != ResourceUsage::ReadbackHeap), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Upload && desc.m_usage != ResourceUsage::UploadHeap), "");
  return m_device->createBuffer(heap, desc);
}
Texture GpuDevice::createTexture(ResourceHeap& heap, ResourceDescriptor desc)
{
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Default && desc.m_usage != ResourceUsage::GpuOnly), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Readback && desc.m_usage != ResourceUsage::ReadbackHeap), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Upload && desc.m_usage != ResourceUsage::UploadHeap), "");
  return m_device->createTexture(heap, desc);
}
// shader views
TextureSRV GpuDevice::createTextureSRV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureSRV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.getTexture(), ResourceShaderType::ShaderView, viewDesc);
  return view;
}
TextureUAV GpuDevice::createTextureUAV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureUAV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.getTexture(), ResourceShaderType::UnorderedAccess, viewDesc);
  return view;
}
TextureRTV GpuDevice::createTextureRTV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureRTV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.getTexture(), ResourceShaderType::RenderTarget, viewDesc);
  return view;
}
TextureDSV GpuDevice::createTextureDSV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureDSV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.getTexture(), ResourceShaderType::DepthStencil, viewDesc);
  return view;
}

BufferSRV GpuDevice::createBufferSRV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferSRV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), ResourceShaderType::ShaderView, viewDesc);
  return view;
}
BufferUAV GpuDevice::createBufferUAV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferUAV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), ResourceShaderType::UnorderedAccess, viewDesc);
  return view;
}
BufferCBV GpuDevice::createBufferCBV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferCBV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), ResourceShaderType::Unknown, viewDesc);
  return view;
}
BufferIBV GpuDevice::createBufferIBV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferIBV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), ResourceShaderType::Unknown, viewDesc);
  return view;
}

bool GpuDevice::isValid()
{
  return m_device->isValid();
}

void GpuDevice::submit(GraphicsCmdBuffer& gfx)
{
  LiveCmdBuffer element;
  element.cmdBuffer = gfx;

  {
    auto sequence = gfx.m_seqNum;
    auto index = m_fenceAllocator.nextRange(sequence, 1);
    while (index.count() == 0)
    {
      F_ASSERT(m_liveCmdBuffers.size() != 0, "No commandbuffers live but still cannot acquire free Fence.");
      updateCompletedSequences();
      index = m_fenceAllocator.nextRange(sequence, 1);
    }

    element.fence = m_rawFences.at(index.start());
    m_device->resetFence(element.fence);
  }

  gfx.prepareForSubmit(*m_device);
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
  for (auto&& it : m_liveCmdBuffers)
  {
    if (it.cmdBuffer.m_seqNum == fence.get())
    {
      m_device->waitFence(it.fence);
      break;
    }
  }
  updateCompletedSequences();
}
void GpuDevice::waitIdle()
{
  m_device->waitIdle();
  
  while (!m_liveCmdBuffers.empty())
  {
    {
      auto&& livecmdBuffer = m_liveCmdBuffers.front();
      m_device->waitFence(livecmdBuffer.fence);
      m_tracker.complete(livecmdBuffer.cmdBuffer.m_seqNum);
    }
    m_liveCmdBuffers.pop_front();
  }
  
  updateCompletedSequences();
}

void GpuDevice::updateCompletedSequences()
{
  while (!m_liveCmdBuffers.empty())
  {
    {
      auto&& livecmdBuffer = m_liveCmdBuffers.front();
      if (!m_device->checkFence(livecmdBuffer.fence))
      {
        break;
      }
      m_tracker.complete(livecmdBuffer.cmdBuffer.m_seqNum);
      // TODO: mark cmdbuffer as "reset before using"
    }
    m_liveCmdBuffers.pop_front();
  }

  // update various managers

  m_cmdBufferAllocator.sequenceCompleted([&](faze::SeqNum num)
  {
    return m_tracker.hasCompleted(num);
  });

  m_fenceAllocator.sequenceCompleted([&](faze::SeqNum num)
  {
    return m_tracker.hasCompleted(num);
  });
}

void GpuDevice::destroyResources()
{
  for (auto&& it : m_descriptorPools)
  {
    m_device->destroy(it.impl());
  }
}