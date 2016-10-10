#include "VulkanGpuDevice.hpp"

#include "VulkanFormat.hpp"

VulkanGpuDevice::VulkanGpuDevice(
  std::shared_ptr<vk::Device> device
  , vk::PhysicalDevice physDev
  , FileSystem& fs
  , vk::AllocationCallbacks alloc_info
  , std::vector<vk::QueueFamilyProperties> queues
  , vk::PhysicalDeviceMemoryProperties memProp
  , bool debugLayer)
  : m_alloc_info(alloc_info)
  , m_device(device)
  , m_physDevice(physDev)
  , m_debugLayer(debugLayer)
  , m_queues(queues)
  , m_singleQueue(false)
  , m_computeQueues(false)
  , m_dmaQueues(false)
  , m_graphicQueues(false)
  , m_uma(false)
  , m_shaders(fs, "../vkShaders", "spirv")
  , m_freeQueueIndexes(std::make_shared<FreeQueues>())
{
  // try to figure out unique queues, abort or something when finding unsupported count.
  // universal
  // graphics+compute
  // compute
  // dma
  size_t totalQueues = 0;
  for (int k = 0; k < static_cast<int>(m_queues.size()); ++k)
  {
    constexpr auto GfxQ = static_cast<uint32_t>(VK_QUEUE_GRAPHICS_BIT);
    constexpr auto CpQ = static_cast<uint32_t>(VK_QUEUE_COMPUTE_BIT);
    constexpr auto DMAQ = static_cast<uint32_t>(VK_QUEUE_TRANSFER_BIT);
    auto&& it = m_queues[k];
    auto current = static_cast<uint32_t>(it.queueFlags);
    if ((current & (GfxQ | CpQ | DMAQ)) == GfxQ+CpQ+DMAQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes->universal.push_back(i);
      }
      m_freeQueueIndexes->universalIndex = k;
    }
    else if ((current & (GfxQ | CpQ)) == GfxQ + CpQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes->graphics.push_back(i);
      }
      m_freeQueueIndexes->graphicsIndex = k;
    }
    else if ((current & CpQ) == CpQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes->compute.push_back(i);
      }
      m_freeQueueIndexes->computeIndex = k;
    }
    else if ((current & DMAQ) == DMAQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes->dma.push_back(i);
      }
      m_freeQueueIndexes->dmaIndex = k;
    }
    totalQueues += it.queueCount;
  }
  if (totalQueues == 0)
  {
    F_ERROR("wtf, not sane device.");
  }
  else if (totalQueues == 1)
  {
    // single queue =_=, IIIINNNTTTEEEELLL
    m_singleQueue = true;
    // lets just fetch it and copy it for those who need it.
    //auto que = m_device->getQueue(0, 0); // TODO: 0 index is wrong.
	  m_internalUniversalQueue = std::make_shared<vk::Queue>(m_device->getQueue(0, 0));
  }
  if (m_freeQueueIndexes->universal.size() > 0
    && m_freeQueueIndexes->graphics.size() > 0)
  {
    F_ERROR("abort mission. Too many variations of queues.");
  }

  m_computeQueues = !m_freeQueueIndexes->compute.empty();
  m_dmaQueues = !m_freeQueueIndexes->dma.empty();
  m_graphicQueues = !m_freeQueueIndexes->graphics.empty();

  // Heap infos
  auto heapCounts = memProp.memoryHeapCount;
  if (heapCounts == 1 && memProp.memoryHeaps[0].flags == vk::MemoryHeapFlagBits::eDeviceLocal)
  {
    m_uma = true;
  }

  for (unsigned i = 0; i < memProp.memoryHeapCount; ++i)
  {
	  F_ILOG("Graphics/Memory", "memory heap %u: %.3fGB", i, float(memProp.memoryHeaps[i].size) / 1024.f / 1024.f / 1024.f);
  }

  auto memTypeCount = memProp.memoryTypeCount;
  auto memPtr = memProp.memoryTypes;

  auto checkFlagSet = [](vk::MemoryType& type, vk::MemoryPropertyFlagBits flag)
  {
    return (type.propertyFlags & flag) == flag;
  };
  F_ILOG("Graphics/Memory", "heapCount %u memTypeCount %u",memProp.memoryHeapCount, memTypeCount);
  for (int i = 0; i < static_cast<int>(memTypeCount); ++i)
  {
    // TODO probably bug here with flags.
    auto memType = memPtr[i];
    if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eDeviceLocal))
    {
      F_ILOG("Graphics/Memory", "heap %u type %u was eDeviceLocal",memType.heapIndex, i);
      if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
      {
        F_ILOG("Graphics/Memory", "heap %u type %u was eHostVisible",memType.heapIndex, i);
		if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCoherent))
			F_ILOG("Graphics/Memory", "heap %u type %u was eHostCoherent", memType.heapIndex, i);
		if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
			F_ILOG("Graphics/Memory", "heap %u type %u was eHostCached", memType.heapIndex, i);
        // weird memory only for uma... usually
		if (m_memoryTypes.deviceHostIndex == -1)
			m_memoryTypes.deviceHostIndex = i;
      }
      else
      {
        //F_ILOG("MemoryTypeDebug", "type %u was not eHostVisible", i);
		if (m_memoryTypes.deviceLocalIndex == -1) // always first legit index, otherwise errors.
			m_memoryTypes.deviceLocalIndex = i;
      }
    }
    else if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
    {
      F_ILOG("Graphics/Memory", "heap %u type %u was eHostVisible",memType.heapIndex, i);
	  if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCoherent))
		F_ILOG("Graphics/Memory", "heap %u type %u was eHostCoherent",memType.heapIndex, i);
      if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
      {
        F_ILOG("Graphics/Memory", "heap %u type %u was eHostCached",memType.heapIndex, i);
		if (m_memoryTypes.hostCachedIndex == -1)
			m_memoryTypes.hostCachedIndex = i;
      }
      else
      {
        //F_ILOG("MemoryTypeDebug", "type %u was not eHostCached", i);
		  if (m_memoryTypes.hostNormalIndex == -1)
			  m_memoryTypes.hostNormalIndex = i;
      }
    }
  }
  // validify memorytypes
  if (m_memoryTypes.deviceHostIndex != -1 || ((m_memoryTypes.deviceLocalIndex != -1) || (m_memoryTypes.hostNormalIndex != -1)))
  {
    // normal!
	F_ILOG("Graphics/Memory", "deviceHostIndex(UMA) %d", m_memoryTypes.deviceHostIndex);
	F_ILOG("Graphics/Memory", "deviceLocalIndex %d", m_memoryTypes.deviceLocalIndex);
	F_ILOG("Graphics/Memory", "hostNormalIndex %d", m_memoryTypes.hostNormalIndex);
	F_ILOG("Graphics/Memory", "hostCachedIndex %d", m_memoryTypes.hostCachedIndex);
  }
  else
  {
    F_ERROR("not a sane situation."); // not a sane situation.
  }

  // figure out indexes for default, upload, readback...
}


std::vector<ResourceDescriptor> VulkanGpuDevice::querySwapChainInfo(VulkanSurface& surface)
{
	auto surfaceCap = m_physDevice.getSurfaceCapabilitiesKHR(*surface.surface);
	F_SLOG("Graphics/Surface", "surface details\n");
	F_SLOG("Graphics/Surface", "min image Count: %d\n", surfaceCap.minImageCount);
	F_SLOG("Graphics/Surface", "current res %dx%d\n", surfaceCap.currentExtent.width, surfaceCap.currentExtent.height);
	F_SLOG("Graphics/Surface", "min res %dx%d\n", surfaceCap.minImageExtent.width, surfaceCap.minImageExtent.height);

	auto formats = m_physDevice.getSurfaceFormatsKHR(*surface.surface);

	for (auto&& fmt : formats)
	{
		F_SLOG("Graphics/Surface", "format: %s\n", vk::to_string(fmt.format).c_str());
	}

	auto asd = m_physDevice.getSurfacePresentModesKHR(*surface.surface);

	for (auto&& fmt : asd)
	{
		if (fmt == vk::PresentModeKHR::eImmediate)
			F_SLOG("Graphics/AvailablePresentModes", "Immediate\n");
		if (fmt == vk::PresentModeKHR::eMailbox)
			F_SLOG("Graphics/AvailablePresentModes", "Mailbox\n");
		if (fmt == vk::PresentModeKHR::eFifo)
			F_SLOG("Graphics/AvailablePresentModes", "Fifo\n");
		if (fmt == vk::PresentModeKHR::eFifoRelaxed)
			F_SLOG("Graphics/AvailablePresentModes", "FifoRelaxed\n");
	}

  std::vector<ResourceDescriptor> descs;

  for (auto&& format : formats)
  {
    descs.emplace_back(ResourceDescriptor()
      .setArraySize(1)
      .setWidth(surfaceCap.currentExtent.width)
      .setHeight(surfaceCap.currentExtent.height)
      .setFormat(formatToFazeFormat(format.format))
      .setLayout(TextureLayout::Unknown)
      .setDepth(1)
      .setMiplevels(1));
  }
  return descs;
}

VulkanSwapchain VulkanGpuDevice::createSwapchain(VulkanSurface& surface, VulkanQueue& queue, PresentMode mode)
{
	vk::PresentModeKHR khrmode;
	switch (mode)
	{
	case PresentMode::Mailbox:
		khrmode = vk::PresentModeKHR::eMailbox;
		break;
	case PresentMode::Fifo:
		khrmode = vk::PresentModeKHR::eFifo;
		break;
	case PresentMode::FifoRelaxed:
		khrmode = vk::PresentModeKHR::eFifoRelaxed;
		break;
	case PresentMode::Immediate:
	default:
		khrmode = vk::PresentModeKHR::eImmediate;
		break;
	}

	auto surfaceCap = m_physDevice.getSurfaceCapabilitiesKHR(*surface.surface);


	auto extent = surfaceCap.currentExtent;
	if (extent.height == 0)
	{
		extent.height = surfaceCap.minImageExtent.height;
	}
	if (extent.width == 0)
	{
		extent.width = surfaceCap.minImageExtent.width;
	}

	if (!m_physDevice.getSurfaceSupportKHR(queue.m_index, *surface.surface))
	{
		F_ASSERT(false, "Was not supported.");
	}

	vk::SwapchainCreateInfoKHR info = vk::SwapchainCreateInfoKHR()
		.setSurface(*surface.surface)
		.setMinImageCount(surfaceCap.minImageCount)
		.setImageFormat(vk::Format::eB8G8R8A8Unorm)
		.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.setImageExtent(surfaceCap.currentExtent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)  // linear to here
		.setImageSharingMode(vk::SharingMode::eExclusive)
	//	.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eInherit)
	//	.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eInherit)
		.setPresentMode(khrmode)
		.setClipped(false);

	auto swapchain = m_device->createSwapchainKHR(info);

	auto dev = m_device;

	VulkanSwapchain ret;
	ret.m_swapchain = std::shared_ptr<vk::SwapchainKHR>(new vk::SwapchainKHR(swapchain), [dev](vk::SwapchainKHR* sc)
	{
		dev->destroySwapchainKHR(*sc);
	});
	return ret;
}

std::vector<VulkanTexture> VulkanGpuDevice::getSwapchainTextures(VulkanSwapchain& sc)
{
  auto images = m_device->getSwapchainImagesKHR(*sc.m_swapchain);

  std::vector<VulkanTexture> texture;
  for (auto&& image : images)
  {
    texture.push_back(VulkanTexture(m_resourceID++, std::make_shared<vk::Image>(image)));
  }

  return texture;
}

VulkanQueue VulkanGpuDevice::createDMAQueue()
{
	uint32_t queueFamilyIndex = 0;
	uint32_t queueId = 0;
	if (m_singleQueue)
	{
		// we already have queue in this case, just get a copy of it.
		return VulkanQueue(m_internalUniversalQueue, 0);
	}
	else if (m_dmaQueues && !m_freeQueueIndexes->dma.empty())
	{
		// yay, realdeal
		queueFamilyIndex = m_freeQueueIndexes->dmaIndex;
		queueId = m_freeQueueIndexes->dma.back();
		m_freeQueueIndexes->dma.pop_back();
		auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
		return VulkanQueue(std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue* cQueue) { m_freeQueueIndexes->dma.push_back(queueId); delete cQueue; }), queueFamilyIndex);
	}
	if (!m_freeQueueIndexes->universal.empty())
	{
		queueFamilyIndex = m_freeQueueIndexes->universalIndex;
		queueId = m_freeQueueIndexes->universal.back();
		m_freeQueueIndexes->universal.pop_back();
		auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
		return VulkanQueue(std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue* cQueue) { m_freeQueueIndexes->universal.push_back(queueId);  delete cQueue; }), queueFamilyIndex);
	}

	return VulkanQueue(std::shared_ptr<vk::Queue>(nullptr), -1);
}

VulkanQueue VulkanGpuDevice::createComputeQueue()
{
	uint32_t queueFamilyIndex = 0;
	uint32_t queueId = 0;
	if (m_singleQueue)
	{
		// we already have queue in this case, just get a copy of it.
		return VulkanQueue(m_internalUniversalQueue, 0);
	}
	else if (m_computeQueues && !m_freeQueueIndexes->compute.empty())
	{
		// yay, realdeal
		queueFamilyIndex = m_freeQueueIndexes->computeIndex;
		queueId = m_freeQueueIndexes->compute.back();
		m_freeQueueIndexes->compute.pop_back();
		auto que = m_device->getQueue(queueFamilyIndex, queueId);
		return VulkanQueue(std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue* cQueue) { m_freeQueueIndexes->compute.push_back(queueId); delete cQueue; }), queueFamilyIndex);
	}
	if (!m_freeQueueIndexes->universal.empty())
	{
		queueFamilyIndex = m_freeQueueIndexes->universalIndex;
		queueId = m_freeQueueIndexes->universal.back();
		m_freeQueueIndexes->universal.pop_back();
		auto que = m_device->getQueue(queueFamilyIndex, queueId);
		return VulkanQueue(std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue* cQueue) { m_freeQueueIndexes->universal.push_back(queueId); delete cQueue; }), queueFamilyIndex);
	}

	return VulkanQueue(std::shared_ptr<vk::Queue>(nullptr), -1);
}

VulkanQueue VulkanGpuDevice::createGraphicsQueue()
{
	uint32_t queueFamilyIndex = 0;
	uint32_t queueId = 0;
	auto indexes = m_freeQueueIndexes;
	if (m_singleQueue)
	{
		// we already have queue in this case, just get a copy of it.
		return VulkanQueue(m_internalUniversalQueue, 0);
	}
	else if (!indexes->graphics.empty())
	{
		// yay, realdeal
		queueFamilyIndex = indexes->graphicsIndex;
		queueId = indexes->graphics.back();
		indexes->graphics.pop_back();
		auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
		return VulkanQueue(std::shared_ptr<vk::Queue>(new vk::Queue(que), [&, indexes](vk::Queue* cQueue) { indexes->graphics.push_back(queueId); delete cQueue;  }), queueFamilyIndex);
	}
	if (!indexes->universal.empty())
	{
		queueFamilyIndex = indexes->universalIndex;
		queueId = indexes->universal.back();
		indexes->universal.pop_back();
		auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
		return VulkanQueue(std::shared_ptr<vk::Queue>(new vk::Queue(que), [&, indexes](vk::Queue* cQueue)
		{
			indexes->universal.push_back(queueId);
			delete cQueue;
		}), queueFamilyIndex);
	}

	return VulkanQueue(std::shared_ptr<vk::Queue>(nullptr), -1);
}

VulkanCmdBuffer VulkanGpuDevice::createDMACommandBuffer()
{
	int index = m_freeQueueIndexes->universalIndex;
	if (m_dmaQueues)
	{
		index = m_freeQueueIndexes->dmaIndex;
	}
	vk::CommandPoolCreateInfo poolInfo = poolInfo = vk::CommandPoolCreateInfo()
		.setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
		.setQueueFamilyIndex(index);

	auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
	auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(1)
		.setCommandPool(pool)
		.setLevel(vk::CommandBufferLevel::ePrimary));
	std::shared_ptr<vk::CommandBuffer> retBuf(new vk::CommandBuffer(buffer[0]));
	std::shared_ptr<vk::CommandPool> retPool(new vk::CommandPool(pool), [&](vk::CommandPool* pool) { m_device->destroyCommandPool(*pool, m_alloc_info); delete pool;  });
	return VulkanCmdBuffer(retBuf, retPool);
}

VulkanCmdBuffer VulkanGpuDevice::createComputeCommandBuffer()
{
	int index = m_freeQueueIndexes->universalIndex;
	if (m_computeQueues)
	{
		index = m_freeQueueIndexes->computeIndex;
	}
	vk::CommandPoolCreateInfo poolInfo = poolInfo = vk::CommandPoolCreateInfo()
		.setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
		.setQueueFamilyIndex(index);
	auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
	auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(1)
		.setCommandPool(pool)
		.setLevel(vk::CommandBufferLevel::ePrimary));
	std::shared_ptr<vk::CommandBuffer> retBuf(new vk::CommandBuffer(buffer[0]));
	std::shared_ptr<vk::CommandPool> retPool(new vk::CommandPool(pool), [&](vk::CommandPool* pool) { m_device->destroyCommandPool(*pool, m_alloc_info); delete pool; });
	return VulkanCmdBuffer(retBuf, retPool);
}

VulkanCmdBuffer VulkanGpuDevice::createGraphicsCommandBuffer()
{
	int index = m_freeQueueIndexes->universalIndex;
	if (m_graphicQueues)
	{
		index = m_freeQueueIndexes->graphicsIndex;
	}
	vk::CommandPoolCreateInfo poolInfo = poolInfo = vk::CommandPoolCreateInfo()
		.setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
		.setQueueFamilyIndex(index);
	auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
	auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(1)
		.setCommandPool(pool)
		.setLevel(vk::CommandBufferLevel::ePrimary));
	std::shared_ptr<vk::CommandBuffer> retBuf(new vk::CommandBuffer(buffer[0]));
	std::shared_ptr<vk::CommandPool> retPool(new vk::CommandPool(pool), [&](vk::CommandPool* pool) { m_device->destroyCommandPool(*pool, m_alloc_info); delete pool; });
	return VulkanCmdBuffer(retBuf, retPool);
}

VulkanDescriptorPool VulkanGpuDevice::createDescriptorPool()
{
  std::vector<vk::DescriptorPoolSize> poolSizes;
  poolSizes.emplace_back(vk::DescriptorPoolSize()
    .setType(vk::DescriptorType::eStorageBufferDynamic)
    .setDescriptorCount(2));
  poolSizes.emplace_back(vk::DescriptorPoolSize()
    .setType(vk::DescriptorType::eStorageBuffer)
    .setDescriptorCount(2000));
  poolSizes.emplace_back(vk::DescriptorPoolSize()
    .setType(vk::DescriptorType::eSampledImage)
    .setDescriptorCount(200));
  poolSizes.emplace_back(vk::DescriptorPoolSize()
    .setType(vk::DescriptorType::eStorageImage)
    .setDescriptorCount(200));
  
  vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
    .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
    .setPPoolSizes(poolSizes.data())
    .setMaxSets(2000);

  auto pool = m_device->createDescriptorPool(info);

  return VulkanDescriptorPool(pool);
}

bool VulkanGpuDevice::isValid()
{
  return m_device.get() != nullptr;
}


VulkanMemoryHeap VulkanGpuDevice::createMemoryHeap(HeapDescriptor desc)
{
  vk::MemoryAllocateInfo allocInfo;
  if (m_uma)
  {
    if (m_memoryTypes.deviceHostIndex != -1)
    {
      allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(desc.m_sizeInBytes)
        .setMemoryTypeIndex(static_cast<uint32_t>(m_memoryTypes.deviceHostIndex));
    }
    else
    {
      F_ERROR("uma but no memory type can be used");
    }
  }
  else
  {
    uint32_t memoryTypeIndex = 0;
    if ((desc.m_heapType == HeapType::Default) && m_memoryTypes.deviceLocalIndex != -1)
    {
      memoryTypeIndex = m_memoryTypes.deviceLocalIndex;
    }
    else if (desc.m_heapType == HeapType::Readback && m_memoryTypes.hostNormalIndex != -1)
    {
      memoryTypeIndex = m_memoryTypes.hostNormalIndex;
    }
    else if (desc.m_heapType == HeapType::Upload && m_memoryTypes.hostCachedIndex != -1)
    {
      memoryTypeIndex = m_memoryTypes.hostCachedIndex;
    }
    else
    {
      F_ERROR("normal device but no valid memory type available");
    }
    allocInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(desc.m_sizeInBytes)
      .setMemoryTypeIndex(static_cast<uint32_t>(memoryTypeIndex));
  }

  auto memory = m_device->allocateMemory(allocInfo, m_alloc_info);
  auto ret = std::shared_ptr<vk::DeviceMemory>(new vk::DeviceMemory, [&](vk::DeviceMemory* memory)
  {
    m_device->freeMemory(*memory);
	  delete memory;
  });
  *ret = memory;
  return VulkanMemoryHeap(ret, desc);
}

VulkanBuffer VulkanGpuDevice::createBuffer(ResourceHeap& heap, ResourceDescriptor desc)
{
  auto bufSize = desc.m_stride*desc.m_width;
  F_ASSERT(bufSize != 0, "Cannot create zero sized buffers.");
  vk::BufferCreateInfo info = vk::BufferCreateInfo()
	  .setSharingMode(vk::SharingMode::eExclusive);
  vk::BufferUsageFlags usageBits = vk::BufferUsageFlagBits::eUniformBuffer;

  if (desc.m_unorderedaccess)
  {
    usageBits = vk::BufferUsageFlagBits::eStorageBuffer;
  }

  auto usage = desc.m_usage;
  if (usage == ResourceUsage::ReadbackHeap)
  {
    usageBits = usageBits | vk::BufferUsageFlagBits::eTransferDst;
  }
  else if (usage == ResourceUsage::UploadHeap)
  {
    usageBits = usageBits | vk::BufferUsageFlagBits::eTransferSrc;
  }
  else
  {
    usageBits = usageBits | vk::BufferUsageFlagBits::eTransferSrc;
    usageBits = usageBits | vk::BufferUsageFlagBits::eTransferDst;
  }
  info = info.setUsage(usageBits);
  info = info.setSize(bufSize);
  auto buffer = m_device->createBuffer(info, m_alloc_info);

  auto pagesNeeded = bufSize / heap.desc().m_alignment + 1;
  auto offset = heap.allocatePages(bufSize);
  F_ASSERT(offset >= 0, "not enough space in heap");

  auto reqs = m_device->getBufferMemoryRequirements(buffer);
  if (reqs.size > pagesNeeded*heap.desc().m_alignment || heap.desc().m_alignment % reqs.alignment != 0)
  {
    F_ASSERT(false, "wtf!");
  }
  auto memory = heap.impl().m_resource;
  m_device->bindBufferMemory(buffer, *heap.impl().m_resource, offset);
  auto offsetPage = offset / heap.desc().m_alignment;
  auto heapCopy = heap;
  auto ret = std::shared_ptr<vk::Buffer>(new vk::Buffer(buffer), [&, offsetPage, pagesNeeded, heapCopy](vk::Buffer* buffer)
  {
    if (heapCopy.isValid() && buffer)
    {
      heap.freePages(offsetPage, pagesNeeded);
      m_device->destroyBuffer(*buffer, m_alloc_info);
      delete buffer;
    }
  });

  std::function<RawMapping(int64_t, int64_t)> mapper = [&, usage, offset, memory](int64_t offsetIntoBuffer, int64_t size)
  {
    // insert some mapping asserts here
    F_ASSERT(usage == ResourceUsage::UploadHeap || usage == ResourceUsage::ReadbackHeap, "cannot map device memory");
    RawMapping mapped;
    auto mapping = m_device->mapMemory(*memory, offset + offsetIntoBuffer, size, vk::MemoryMapFlags());
    std::shared_ptr<void> target(mapping,
      [&, memory](void*) -> void
    {
      m_device->unmapMemory(*memory);
    });
    mapped.mapped = target;

    return mapped;
  };
  auto buf = VulkanBuffer(m_resourceID++, ret, bufSize);
  buf.m_mapResource = mapper;
  return buf;
}
VulkanTexture VulkanGpuDevice::createTexture(ResourceHeap& , ResourceDescriptor descriptor)
{

  vk::ImageType imageType = vk::ImageType::e2D;

  vk::ImageCreateInfo info = vk::ImageCreateInfo()
    .setArrayLayers(descriptor.m_arraySize)
    .setExtent(vk::Extent3D()
      .setWidth(descriptor.m_width)
      .setHeight(descriptor.m_height)
      .setDepth(descriptor.m_depth))
    .setFormat(formatToVkFormat[descriptor.m_format].view)
    .setImageType(imageType)
    .setInitialLayout(vk::ImageLayout::eUndefined)
    .setMipLevels(descriptor.m_miplevels)
    .setSamples(vk::SampleCountFlagBits::e1)
    .setTiling(vk::ImageTiling::eOptimal)
    .setUsage(vk::ImageUsageFlagBits::eSampled)
    .setSharingMode(vk::SharingMode::eExclusive);
  auto image = m_device->createImage(info);

  auto reqs = m_device->getImageMemoryRequirements(image);

  F_SLOG("Graphics", "lol %zu %zu %u\n", reqs.size, reqs.alignment, reqs.memoryTypeBits);

  m_device->destroyImage(image);

  return VulkanTexture();
}
// shader views
VulkanBufferShaderView VulkanGpuDevice::createBufferView(VulkanBuffer& buffer, ResourceDescriptor& desc, ResourceShaderType shaderType,  ShaderViewDescriptor descriptor)
{
  auto elementSize = desc.m_stride;
  auto sizeInElements = desc.m_width;
  auto firstElement = descriptor.m_firstElement*elementSize;
  auto maxRange = descriptor.m_elementCount*elementSize;
  if (descriptor.m_elementCount <= 0)
	  maxRange = elementSize*sizeInElements; // VK_WHOLE_SIZE

  vk::DescriptorType type = vk::DescriptorType::eStorageBuffer;
  if (shaderType == ResourceShaderType::ShaderView)
  {
	  type = vk::DescriptorType::eStorageBuffer;
  }
  else if (shaderType == ResourceShaderType::UnorderedAccess)
  {
	  type = vk::DescriptorType::eStorageBuffer;
  }
  return VulkanBufferShaderView(vk::DescriptorBufferInfo()
	  .setBuffer(*buffer.m_resource)
	  .setOffset(firstElement)
	  .setRange(maxRange)
  , type, buffer.m_state, buffer.uniqueId);
}

VulkanTextureShaderView VulkanGpuDevice::createTextureView(VulkanTexture& texture, ResourceDescriptor& , ResourceShaderType ,  ShaderViewDescriptor)
{
  return VulkanTextureShaderView(vk::DescriptorImageInfo(), vk::DescriptorType::eSampledImage, texture.m_state, texture.uniqueId);
}

VulkanPipeline VulkanGpuDevice::createGraphicsPipeline(GraphicsPipelineDescriptor )
{
  return VulkanPipeline();
}

VulkanPipeline VulkanGpuDevice::createComputePipeline(ShaderInputLayout shaderLayout, ComputePipelineDescriptor desc)
{

  vk::ShaderModule readyShader = m_shaders.shader(*m_device, desc.shader(), ShaderStorage::ShaderType::Compute);
 
  unsigned indexStart = 1;
  // 
  std::vector<vk::DescriptorSetLayoutBinding> bindings;

  bindings.push_back(vk::DescriptorSetLayoutBinding()
	  .setBinding(0)
	  .setDescriptorCount(1)
	  .setDescriptorType(vk::DescriptorType::eStorageBuffer)
	  .setStageFlags(vk::ShaderStageFlagBits::eCompute));

  // read only buffers
  for (int localIndex = 0; localIndex < shaderLayout.srvBufferCount; ++localIndex)
  {
	  bindings.push_back(vk::DescriptorSetLayoutBinding()
		  .setBinding(indexStart)
		  .setDescriptorCount(1)
		  .setDescriptorType(vk::DescriptorType::eStorageBuffer)
		  .setStageFlags(vk::ShaderStageFlagBits::eCompute));
	  indexStart += 1;
  }

  // read/write buffers
  for (int localIndex = 0; localIndex < shaderLayout.uavBufferCount; ++localIndex)
  {
	  bindings.push_back(vk::DescriptorSetLayoutBinding()
		  .setBinding(indexStart)
		  .setDescriptorCount(1)
		  .setDescriptorType(vk::DescriptorType::eStorageBuffer)
		  .setStageFlags(vk::ShaderStageFlagBits::eCompute));
	  indexStart += 1;
  }

  for (int localIndex = 0; localIndex < shaderLayout.srvTextureCount; ++localIndex)
  {
	  bindings.push_back(vk::DescriptorSetLayoutBinding()
		  .setBinding(indexStart)
		  .setDescriptorCount(1)
		  .setDescriptorType(vk::DescriptorType::eSampledImage)
		  .setStageFlags(vk::ShaderStageFlagBits::eCompute));
	  indexStart += 1;
  }

  for (int localIndex = 0; localIndex < shaderLayout.uavTextureCount; ++localIndex)
  {

	  bindings.push_back(vk::DescriptorSetLayoutBinding()
		  .setBinding(indexStart)
		  .setDescriptorCount(4)
		  .setDescriptorType(vk::DescriptorType::eStorageImage)
		  .setStageFlags(vk::ShaderStageFlagBits::eCompute));
	  indexStart += 1;
  }

  vk::DescriptorSetLayoutCreateInfo sampleLayout = vk::DescriptorSetLayoutCreateInfo()
    .setPBindings(bindings.data())
    .setBindingCount(static_cast<uint32_t>(bindings.size()));

  vk::DescriptorSetLayout descriptorSetLayout;

  descriptorSetLayout = m_device->createDescriptorSetLayout(sampleLayout);
  vk::PushConstantRange range = vk::PushConstantRange()
	  .setStageFlags(vk::ShaderStageFlagBits::eCompute)
	  .setOffset(0)
	  .setSize(static_cast<uint32_t>(shaderLayout.pushConstantsSize));
  auto layoutInfo = vk::PipelineLayoutCreateInfo()
	.setPushConstantRangeCount(1)
	.setPPushConstantRanges(&range)
    .setPSetLayouts(&descriptorSetLayout)
    .setSetLayoutCount(1);
  auto layout = m_device->createPipelineLayout(layoutInfo);

  //auto specialiInfo = vk::SpecializationInfo(); // specialisation constant control, needs spirv id information 
  vk::PipelineShaderStageCreateInfo shaderInfo = vk::PipelineShaderStageCreateInfo()
    //.pSpecializationInfo(&specialiInfo)
    .setStage(vk::ShaderStageFlagBits::eCompute)
    .setPName("main")
    .setModule(readyShader);


  auto info = vk::ComputePipelineCreateInfo()
    //.flags(vk::PipelineCreateFlagBits::)
    .setLayout(layout)
    .setStage(shaderInfo);

  vk::PipelineCache invalidCache;

  std::vector<vk::ComputePipelineCreateInfo> infos = {info};

  auto results = m_device->createComputePipelines(invalidCache, infos);

  auto pipeline = std::shared_ptr<vk::Pipeline>(new vk::Pipeline(results[0]), [&](vk::Pipeline* pipeline)
  {
    m_device->destroyPipeline(*pipeline);
	delete pipeline;
  });

  auto pipelineLayout = std::shared_ptr<vk::PipelineLayout>(new vk::PipelineLayout(layout), [&](vk::PipelineLayout* layout)
  {
    m_device->destroyPipelineLayout(*layout);
	delete layout;
  });

  auto descriptorSetLayoutPtr = std::shared_ptr<vk::DescriptorSetLayout>(new vk::DescriptorSetLayout(descriptorSetLayout), [&](vk::DescriptorSetLayout* descriptorSetLayout)
  {
    m_device->destroyDescriptorSetLayout(*descriptorSetLayout);
	delete descriptorSetLayout;
  });

  m_device->destroyShaderModule(readyShader);

  return VulkanPipeline(pipeline, pipelineLayout, descriptorSetLayoutPtr, desc);
}

VulkanFence VulkanGpuDevice::createFence()
{
  auto createInfo = vk::FenceCreateInfo()
    .setFlags(vk::FenceCreateFlagBits::eSignaled);
  auto fence = m_device->createFence(createInfo);

  auto fencePtr = std::shared_ptr<vk::Fence>(new vk::Fence(fence), [&](vk::Fence* fence)
  {
    m_device->destroyFence(*fence);
    delete fence;
  });
  return VulkanFence(fencePtr);
}

void VulkanGpuDevice::waitFence(VulkanFence& fence)
{
  vk::ArrayProxy<const vk::Fence> proxy(*fence.m_fence);
  auto res = m_device->waitForFences(proxy, 1, (std::numeric_limits<int64_t>::max)());
  F_ASSERT(res == vk::Result::eSuccess, "uups");
}

bool VulkanGpuDevice::checkFence(VulkanFence& fence)
{
  auto status = m_device->getFenceStatus(*fence.m_fence);
  
  return status == vk::Result::eSuccess;
}

void VulkanGpuDevice::waitIdle()
{
  m_device->waitIdle();
}

// resets

void VulkanGpuDevice::resetCmdBuffer(VulkanCmdBuffer& buffer)
{
  buffer.m_commandList->hardClear();
  buffer.tracker.reset();
  m_device->resetCommandPool(*buffer.m_pool, vk::CommandPoolResetFlagBits::eReleaseResources);
}

void VulkanGpuDevice::resetFence(VulkanFence& fence)
{
  vk::ArrayProxy<const vk::Fence> proxy(*fence.m_fence);
  m_device->resetFences(proxy);
}

void VulkanGpuDevice::reset(VulkanDescriptorPool& pool)
{
  m_device->resetDescriptorPool(pool.pool);
}

void VulkanGpuDevice::destroy(VulkanDescriptorPool& pool)
{
  m_device->destroyDescriptorPool(pool.pool);
}
