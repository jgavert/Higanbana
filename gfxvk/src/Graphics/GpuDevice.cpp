#include "GpuDevice.hpp"

#define COMMANDLISTCOUNT 30

GpuDevice::GpuDevice(GpuDeviceImpl device)
  : m_device(std::make_shared<GpuDeviceImpl>(device))
  , m_queue(device.createGraphicsQueue())
  , m_cmdBufferAllocator(COMMANDLISTCOUNT)
  , m_fenceAllocator(COMMANDLISTCOUNT)
{
  for (int64_t i = 0; i < COMMANDLISTCOUNT; i++)
  {
    m_rawCommandBuffers.emplace_back(std::make_shared<CmdBufferImpl>(m_device->createGraphicsCommandBuffer()));
  }

  for (int64_t i = 0; i < COMMANDLISTCOUNT; i++)
  {
    m_rawFences.emplace_back(m_device->createFence());
  }

  for (int64_t i = 0; i < COMMANDLISTCOUNT; i++)
  {
	  m_descriptorPools.emplace_back(DescriptorPool(std::make_shared<DescriptorPoolImpl>(m_device->createDescriptorPool())));
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

std::vector<ResourceDescriptor> GpuDevice::querySwapChainInfo(WindowSurface& surface)
{
	return m_device->querySwapChainInfo(surface.impl);
}

std::vector<PresentMode> GpuDevice::queryPresentModes(WindowSurface& surface)
{
  return m_device->queryPresentModes(surface.impl);
}

Swapchain GpuDevice::createSwapchain(WindowSurface& surface, PresentMode mode, ResourceDescriptor chosen)
{
  ResourceDescriptor usedAsBase = chosen;
  if (chosen.m_format == FormatType::Unknown)
  {
    // figure out ourselves
    auto toChooseFrom = querySwapChainInfo(surface);
    for (auto&& obj : toChooseFrom)
    {
      if (obj.m_format == FormatType::B8G8R8A8)
      {
        usedAsBase = obj;
        break;
      }
    }
    F_ASSERT(usedAsBase.m_format != FormatType::Unknown, "we didn't find proper swapchain format.");
  }

  // check mode given
  auto modes = m_device->queryPresentModes(surface.impl);
  F_ASSERT(!modes.empty(), "no modes available!?");
  PresentMode chosenMode = modes[0];
  for (auto&& validMode : modes)
  {
    if (validMode == mode)
    {
      chosenMode = mode;
    }
  }


	auto impl = m_device->createSwapchain(surface.impl,m_queue, usedAsBase.m_format, chosenMode);
  auto scImages = m_device->getSwapchainTextures(impl);

  std::vector<TextureRTV> finalImages;


  for (auto&& image : scImages)
  {
    TextureRTV rtv;
    rtv.m_view = m_device->createTextureView(image, usedAsBase, ResourceShaderType::RenderTarget);
    rtv.m_texture = Texture(image, usedAsBase);
    finalImages.emplace_back(rtv);
  }

  SemaphoreImpl pre = m_device->createSemaphore();
  SemaphoreImpl post = m_device->createSemaphore();

	return Swapchain(impl, finalImages, pre, post, usedAsBase, chosenMode);
}

void GpuDevice::reCreateSwapchain(Swapchain& sc, WindowSurface& surface, PresentMode mode, ResourceDescriptor chosen)
{
  waitIdle();

  if (mode == PresentMode::Unknown)
  {
    mode = sc.m_mode;
  }

  if (chosen.m_format == FormatType::Unknown)
  {
    chosen = sc.m_descriptor;
  }

  m_device->reCreateSwapchain(sc.m_swapchain, surface.impl, m_queue, chosen, mode);
  auto scImages = m_device->getSwapchainTextures(sc.m_swapchain);

  std::vector<TextureRTV> finalImages;

  for (auto&& image : scImages)
  {
    TextureRTV rtv;
    rtv.m_view = m_device->createTextureView(image, chosen, ResourceShaderType::RenderTarget);
    rtv.m_texture = Texture(image, chosen);
    finalImages.emplace_back(rtv);
  }

  sc.m_descriptor = chosen;
  sc.m_mode = mode;
  sc.m_resources = finalImages;
}

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
  m_device->resetCmdBuffer(*cmdBuffer);
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
  return Buffer(m_device->createBuffer(heap, desc), desc);
}
Texture GpuDevice::createTexture(ResourceHeap& heap, ResourceDescriptor desc)
{
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Default && desc.m_usage != ResourceUsage::GpuOnly), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Readback && desc.m_usage != ResourceUsage::ReadbackHeap), "");
  F_ASSERT(!(heap.desc().m_heapType == HeapType::Upload && desc.m_usage != ResourceUsage::UploadHeap), "");
  return Texture(m_device->createTexture(heap, desc), desc);
}
// shader views
TextureSRV GpuDevice::createTextureSRV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureSRV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.impl(), targetTexture.desc(), ResourceShaderType::ShaderView, viewDesc);
  return view;
}
TextureUAV GpuDevice::createTextureUAV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureUAV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.impl(), targetTexture.desc(), ResourceShaderType::UnorderedAccess, viewDesc);
  return view;
}
TextureRTV GpuDevice::createTextureRTV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureRTV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.impl(), targetTexture.desc(), ResourceShaderType::RenderTarget, viewDesc);
  return view;
}
TextureDSV GpuDevice::createTextureDSV(Texture targetTexture, ShaderViewDescriptor viewDesc)
{
  TextureDSV view;
  view.m_texture = targetTexture;
  view.m_view = m_device->createTextureView(targetTexture.impl(), targetTexture.desc(), ResourceShaderType::DepthStencil, viewDesc);
  return view;
}

BufferSRV GpuDevice::createBufferSRV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferSRV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), targetBuffer.desc(), ResourceShaderType::ShaderView, viewDesc);
  return view;
}
BufferUAV GpuDevice::createBufferUAV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferUAV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), targetBuffer.desc(), ResourceShaderType::UnorderedAccess, viewDesc);
  return view;
}
BufferCBV GpuDevice::createBufferCBV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferCBV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), targetBuffer.desc(), ResourceShaderType::Unknown, viewDesc);
  return view;
}
BufferIBV GpuDevice::createBufferIBV(Buffer targetBuffer, ShaderViewDescriptor viewDesc)
{
  BufferIBV view;
  view.m_buffer = targetBuffer;
  view.m_view = m_device->createBufferView(targetBuffer.getBuffer(), targetBuffer.desc(), ResourceShaderType::Unknown, viewDesc);
  return view;
}

bool GpuDevice::isValid()
{
  return m_device->isValid();
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

TextureRTV GpuDevice::acquirePresentableImage(Swapchain& sc)
{
  int index = m_device->acquireNextImage(sc.m_swapchain, sc.m_pre);
  sc.m_currentIndex = index;
  return sc[index];
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
	m_queue.submit(*gfx.m_cmdBuffer, element.fence);
	m_liveCmdBuffers.emplace_back(element);
}

void GpuDevice::submitSwapchain(GraphicsCmdBuffer& gfx, Swapchain& sc)
{
	gfx.prepareForPresent(sc[sc.lastAcquiredIndex()].texture());

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
	m_queue.submitWithSemaphore(*gfx.m_cmdBuffer, element.fence, sc.m_pre, sc.m_post);
	m_liveCmdBuffers.emplace_back(element);
}

void GpuDevice::present(Swapchain& sc)
{
  m_queue.present(sc.m_swapchain, sc.m_post, sc.lastAcquiredIndex());
}