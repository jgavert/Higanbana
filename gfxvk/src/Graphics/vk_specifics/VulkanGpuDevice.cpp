#include "VulkanGpuDevice.hpp"

VulkanGpuDevice::VulkanGpuDevice(
  FazPtrVk<vk::Device> device,
  vk::AllocationCallbacks alloc_info,
  std::vector<vk::QueueFamilyProperties> queues,
  vk::PhysicalDeviceMemoryProperties memProp,
  bool debugLayer)
  : m_alloc_info(alloc_info)
  , m_device(device)
  , m_debugLayer(debugLayer)
  , m_queues(queues)
  , m_singleQueue(false)
  , m_computeQueues(false)
  , m_dmaQueues(false)
  , m_graphicQueues(false)
  , m_freeQueueIndexes({})
  , m_uma(false)
  , m_memoryTypes({-1, -1, -1, -1})
{
  // try to figure out unique queues, abort or something when finding unsupported count.
  // universal
  // graphics+compute
  // compute
  // dma
  size_t totalQueues = 0;
  for (int k = 0; k < m_queues.size(); ++k)
  {
    constexpr auto GfxQ = static_cast<uint32_t>(VK_QUEUE_GRAPHICS_BIT);
    constexpr auto CpQ = static_cast<uint32_t>(VK_QUEUE_COMPUTE_BIT);
    constexpr auto DMAQ = static_cast<uint32_t>(VK_QUEUE_TRANSFER_BIT);
    auto&& it = m_queues[k];
    auto current = static_cast<uint32_t>(it.queueFlags());
    if ((current & (GfxQ | CpQ | DMAQ)) == GfxQ+CpQ+DMAQ)
    {
      for (uint32_t i = 0; i < it.queueCount(); ++i)
      {
        m_freeQueueIndexes.universal.push_back(i);
      }
      m_freeQueueIndexes.universalIndex = k;
    }
    else if ((current & (GfxQ | CpQ)) == GfxQ + CpQ)
    {
      for (uint32_t i = 0; i < it.queueCount(); ++i)
      {
        m_freeQueueIndexes.graphics.push_back(i);
      }
      m_freeQueueIndexes.graphicsIndex = k;
    }
    else if ((current & CpQ) == CpQ)
    {
      for (uint32_t i = 0; i < it.queueCount(); ++i)
      {
        m_freeQueueIndexes.compute.push_back(i);
      }
      m_freeQueueIndexes.computeIndex = k;
    }
    else if ((current & DMAQ) == DMAQ)
    {
      for (uint32_t i = 0; i < it.queueCount(); ++i)
      {
        m_freeQueueIndexes.dma.push_back(i);
      }
      m_freeQueueIndexes.dmaIndex = k;
    }
    totalQueues += it.queueCount();
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
    auto que = m_device->getQueue(0, 0); // TODO: 0 index is wrong.
    m_internalUniversalQueue = FazPtrVk<vk::Queue>(que, [](vk::Queue) {}, false);
  }
  if (m_freeQueueIndexes.universal.size() > 0
    && m_freeQueueIndexes.graphics.size() > 0)
  {
    F_ERROR("abort mission. Too many variations of queues.");
  }

  m_computeQueues = !m_freeQueueIndexes.compute.empty();
  m_dmaQueues = !m_freeQueueIndexes.dma.empty();
  m_graphicQueues = !m_freeQueueIndexes.graphics.empty();

  // Heap infos
  auto heapCounts = memProp.memoryHeapCount();
  if (heapCounts == 1 && memProp.memoryHeaps()[0].flags() == vk::MemoryHeapFlagBits::eDeviceLocal)
  {
    m_uma = true;
  }

  auto memTypeCount = memProp.memoryTypeCount();
  auto memPtr = memProp.memoryTypes();

  auto checkFlagSet = [](vk::MemoryType& type, vk::MemoryPropertyFlagBits flag)
  {
    return (type.propertyFlags() & flag) == flag;
  };

  for (int i = 0; i < static_cast<int>(memTypeCount); ++i)
  {
    // TODO probably bug here with flags.
    auto memType = memPtr[i];
    if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eDeviceLocal))
    {
      if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
      {
        // weird memory only for uma... usually
        m_memoryTypes.deviceHostIndex = i;
      }
      else
      {
        m_memoryTypes.deviceLocalIndex = i;
      }
    }
    else if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
    {
      if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
      {
        m_memoryTypes.hostCachedIndex = i;
      }
      else
      {
        m_memoryTypes.hostNormalIndex = i;
      }
    }
  }
  // validify memorytypes
  if (m_memoryTypes.deviceHostIndex != 0 || ((m_memoryTypes.deviceLocalIndex != -1) || (m_memoryTypes.hostNormalIndex != -1)))
  {
    // normal!
  }
  else
  {
    F_ERROR("not sane situation."); // not sane situation.
  }

  // figure out indexes for default, upload, readback...
}

VulkanQueue VulkanGpuDevice::createDMAQueue()
{
  uint32_t queueFamilyIndex = 0;
  uint32_t queueId = 0;
  if (m_singleQueue)
  {
    // we already have queue in this case, just get a copy of it.
    return m_internalUniversalQueue;
  }
  else if (m_dmaQueues && !m_freeQueueIndexes.dma.empty())
  {
    // yay, realdeal
    queueFamilyIndex = m_freeQueueIndexes.dmaIndex;
    queueId = m_freeQueueIndexes.dma.back();
    m_freeQueueIndexes.dma.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.dma.push_back(queueId); });
  }
  if (!m_freeQueueIndexes.universal.empty())
  {
    queueFamilyIndex = m_freeQueueIndexes.universalIndex;
    queueId = m_freeQueueIndexes.universal.back();
    m_freeQueueIndexes.universal.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.universal.push_back(queueId); });
  }

  return FazPtrVk<vk::Queue>(nullptr, [&](vk::Queue) {}, false);
}

VulkanQueue VulkanGpuDevice::createComputeQueue()
{
  uint32_t queueFamilyIndex = 0;
  uint32_t queueId = 0;
  if (m_singleQueue)
  {
    // we already have queue in this case, just get a copy of it.
    return m_internalUniversalQueue;
  }
  else if (m_computeQueues && !m_freeQueueIndexes.compute.empty())
  {
    // yay, realdeal
    queueFamilyIndex = m_freeQueueIndexes.computeIndex;
    queueId = m_freeQueueIndexes.compute.back();
    m_freeQueueIndexes.compute.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId);
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.compute.push_back(queueId); });
  }
  if (!m_freeQueueIndexes.universal.empty())
  {
    queueFamilyIndex = m_freeQueueIndexes.universalIndex;
    queueId = m_freeQueueIndexes.universal.back();
    m_freeQueueIndexes.universal.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId);
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.universal.push_back(queueId); });
  }

  return FazPtrVk<vk::Queue>(nullptr, [&](vk::Queue) {}, false);
}

VulkanQueue VulkanGpuDevice::createGraphicsQueue()
{
  uint32_t queueFamilyIndex = 0;
  uint32_t queueId = 0;
  if (m_singleQueue)
  {
    // we already have queue in this case, just get a copy of it.
    return m_internalUniversalQueue;
  }
  else if (!m_freeQueueIndexes.graphics.empty())
  {
    // yay, realdeal
    queueFamilyIndex = m_freeQueueIndexes.graphicsIndex;
    queueId = m_freeQueueIndexes.graphics.back();
    m_freeQueueIndexes.graphics.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.graphics.push_back(queueId); });
  }
  if (!m_freeQueueIndexes.universal.empty())
  {
    queueFamilyIndex = m_freeQueueIndexes.universalIndex;
    queueId = m_freeQueueIndexes.universal.back();
    m_freeQueueIndexes.universal.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.universal.push_back(queueId); });
  }

  return FazPtrVk<vk::Queue>(nullptr, [&](vk::Queue) {}, false);
}

VulkanCmdBuffer VulkanGpuDevice::createDMACommandBuffer()
{
  vk::CommandPoolCreateInfo poolInfo;
  if (m_dmaQueues)
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .flags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .sType(vk::StructureType::eCommandPoolCreateInfo)
      .queueFamilyIndex(m_freeQueueIndexes.dmaIndex);
  }
  else
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .flags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .sType(vk::StructureType::eCommandPoolCreateInfo)
      .queueFamilyIndex(m_freeQueueIndexes.universalIndex);
  }
  auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
  auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
    .commandBufferCount(1)
    .commandPool(pool)
    .sType(vk::StructureType::eCommandBufferAllocateInfo)
    .level(vk::CommandBufferLevel::ePrimary));
  FazPtrVk<vk::CommandBuffer> retBuf = FazPtrVk<vk::CommandBuffer>(buffer[0], [](vk::CommandBuffer) {});
  FazPtrVk<vk::CommandPool> retPool = FazPtrVk<vk::CommandPool>(pool, [&](vk::CommandPool pool) { m_device->destroyCommandPool(pool, m_alloc_info); });
  return VulkanCmdBuffer(retBuf, retPool);
}

VulkanCmdBuffer VulkanGpuDevice::createComputeCommandBuffer()
{
  vk::CommandPoolCreateInfo poolInfo;
  if (m_computeQueues)
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .flags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .sType(vk::StructureType::eCommandPoolCreateInfo)
      .queueFamilyIndex(m_freeQueueIndexes.computeIndex);
  }
  else
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .flags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .sType(vk::StructureType::eCommandPoolCreateInfo)
      .queueFamilyIndex(m_freeQueueIndexes.universalIndex);
  }
  auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
  auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
    .commandBufferCount(1)
    .commandPool(pool)
    .sType(vk::StructureType::eCommandBufferAllocateInfo)
    .level(vk::CommandBufferLevel::ePrimary));
  FazPtrVk<vk::CommandBuffer> retBuf = FazPtrVk<vk::CommandBuffer>(buffer[0], [](vk::CommandBuffer) {});
  FazPtrVk<vk::CommandPool> retPool = FazPtrVk<vk::CommandPool>(pool, [&](vk::CommandPool pool) { m_device->destroyCommandPool(pool, m_alloc_info); });
  return VulkanCmdBuffer(retBuf, retPool);
}

VulkanCmdBuffer VulkanGpuDevice::createGraphicsCommandBuffer()
{
  vk::CommandPoolCreateInfo poolInfo;
  if (m_graphicQueues)
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .flags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .sType(vk::StructureType::eCommandPoolCreateInfo)
      .queueFamilyIndex(m_freeQueueIndexes.graphicsIndex);
  }
  else
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .flags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .sType(vk::StructureType::eCommandPoolCreateInfo)
      .queueFamilyIndex(m_freeQueueIndexes.universalIndex);
  }
  auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
  auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
    .commandBufferCount(1)
    .commandPool(pool)
    .sType(vk::StructureType::eCommandBufferAllocateInfo)
    .level(vk::CommandBufferLevel::ePrimary));
  FazPtrVk<vk::CommandBuffer> retBuf = FazPtrVk<vk::CommandBuffer>(buffer[0], [](vk::CommandBuffer) {});
  FazPtrVk<vk::CommandPool> retPool = FazPtrVk<vk::CommandPool>(pool, [&](vk::CommandPool pool) { m_device->destroyCommandPool(pool, m_alloc_info); });
  return VulkanCmdBuffer(retBuf, retPool);
}

bool VulkanGpuDevice::isValid()
{
  return m_device.isValid();
}

VulkanMemoryHeap VulkanGpuDevice::createMemoryHeap(HeapDescriptor desc)
{
  vk::MemoryAllocateInfo allocInfo;
  if (m_uma)
  {
    if (m_memoryTypes.deviceHostIndex != -1)
    {
      allocInfo = vk::MemoryAllocateInfo()
        .sType(vk::StructureType::eMemoryAllocateInfo)
        .allocationSize(desc.m_sizeInBytes)
        .memoryTypeIndex(static_cast<uint32_t>(m_memoryTypes.deviceHostIndex));
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
    else if ((desc.m_heapType == HeapType::Readback || desc.m_heapType == HeapType::Upload) && m_memoryTypes.hostNormalIndex != -1)
    {
      memoryTypeIndex = m_memoryTypes.hostNormalIndex;
    }
    else
    {
      F_ERROR("normal device but no valid memory type available");
    }
    allocInfo = vk::MemoryAllocateInfo()
      .sType(vk::StructureType::eMemoryAllocateInfo)
      .allocationSize(desc.m_sizeInBytes)
      .memoryTypeIndex(static_cast<uint32_t>(memoryTypeIndex));
  }

  auto memory = m_device->allocateMemory(allocInfo, m_alloc_info);
  auto ret = FazPtrVk<vk::DeviceMemory>(memory, [&](vk::DeviceMemory memory)
  {
    m_device->freeMemory(memory, m_alloc_info);
  });
  return VulkanMemoryHeap(ret, desc);
}

VulkanBuffer VulkanGpuDevice::createBuffer(ResourceHeap& heap, ResourceDescriptor desc)
{
  auto bufSize = desc.m_stride*desc.m_width;
  F_ASSERT(bufSize != 0, "Cannot create zero sized buffers.");
  vk::BufferCreateInfo info = vk::BufferCreateInfo()
    .sharingMode(vk::SharingMode::eExclusive);
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
  info = info.usage(usageBits);
  info = info.size(bufSize);
  auto buffer = m_device->createBuffer(info, m_alloc_info);

  auto pagesNeeded = bufSize / heap.desc().m_alignment + 1;
  auto offset = heap.allocatePages(bufSize);
  F_ASSERT(offset >= 0, "not enough space in heap");

  auto reqs = m_device->getBufferMemoryRequirements(buffer);
  if (reqs.size() > pagesNeeded*heap.desc().m_alignment || heap.desc().m_alignment % reqs.alignment() != 0)
  {
    F_ASSERT(false, "wtf!");
  }
  auto memory = heap.impl().m_resource;
  m_device->bindBufferMemory(buffer, memory.getRef(), offset);
  auto ret = FazPtrVk<vk::Buffer>(buffer, [&, offset, pagesNeeded, memory](vk::Buffer buffer)
  {
    heap.freePages(offset, pagesNeeded);
    m_device->destroyBuffer(buffer, m_alloc_info);
  });

  std::function<RawMapping(int64_t, int64_t)> mapper = [&, memory, usage](int64_t offsetIntoBuffer, int64_t size)
  {
    // insert some mapping asserts here
    F_ASSERT(usage == ResourceUsage::UploadHeap || usage == ResourceUsage::ReadbackHeap, "cannot map device memory");
    RawMapping mapped;
    auto mapping = m_device->mapMemory(*memory.get(), offset + offsetIntoBuffer, size, vk::MemoryMapFlags());
    FazPtr<uint8_t*> target = FazPtr<uint8_t*>(reinterpret_cast<uint8_t*>(mapping), [&, memory](uint8_t*) -> void 
    {
      m_device->unmapMemory(*memory.get());
    });
    mapped.mapped = target;

    return mapped;
  };
  auto buf = VulkanBuffer(ret, desc);
  buf.m_mapResource = mapper;
  return buf;
}
VulkanTexture VulkanGpuDevice::createTexture(ResourceHeap& , ResourceDescriptor )
{
  return VulkanTexture();
}
// shader views
VulkanBufferShaderView VulkanGpuDevice::createBufferView(VulkanBuffer , ShaderViewDescriptor )
{
  return VulkanBufferShaderView();
}
VulkanTextureShaderView VulkanGpuDevice::createTextureView(VulkanTexture , ShaderViewDescriptor)
{
  return VulkanTextureShaderView();
}

VulkanPipeline VulkanGpuDevice::createGraphicsPipeline(GraphicsPipelineDescriptor )
{
  return VulkanPipeline();
}

VulkanPipeline VulkanGpuDevice::createComputePipeline(ComputePipelineDescriptor )
{
  return VulkanPipeline();
}
