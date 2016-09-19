#include "VulkanGpuDevice.hpp"

VulkanGpuDevice::VulkanGpuDevice(
  std::shared_ptr<vk::Device> device,
  FileSystem& fs,
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
  , m_uma(false)
  , m_shaders(fs, "../vkShaders", "spirv")
  , m_freeQueueIndexes({})
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
    auto current = static_cast<uint32_t>(it.queueFlags);
    if ((current & (GfxQ | CpQ | DMAQ)) == GfxQ+CpQ+DMAQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes.universal.push_back(i);
      }
      m_freeQueueIndexes.universalIndex = k;
    }
    else if ((current & (GfxQ | CpQ)) == GfxQ + CpQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes.graphics.push_back(i);
      }
      m_freeQueueIndexes.graphicsIndex = k;
    }
    else if ((current & CpQ) == CpQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes.compute.push_back(i);
      }
      m_freeQueueIndexes.computeIndex = k;
    }
    else if ((current & DMAQ) == DMAQ)
    {
      for (uint32_t i = 0; i < it.queueCount; ++i)
      {
        m_freeQueueIndexes.dma.push_back(i);
      }
      m_freeQueueIndexes.dmaIndex = k;
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
    auto que = m_device->getQueue(0, 0); // TODO: 0 index is wrong.
	m_internalUniversalQueue = std::make_shared<vk::Queue>(m_device->getQueue(0, 0)); // std::shared_ptr<vk::Queue>(&que);
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
  auto heapCounts = memProp.memoryHeapCount;
  if (heapCounts == 1 && memProp.memoryHeaps[0].flags == vk::MemoryHeapFlagBits::eDeviceLocal)
  {
    m_uma = true;
  }

  auto memTypeCount = memProp.memoryTypeCount;
  auto memPtr = memProp.memoryTypes;

  auto checkFlagSet = [](vk::MemoryType& type, vk::MemoryPropertyFlagBits flag)
  {
    return (type.propertyFlags & flag) == flag;
  };
  F_ILOG("MemoryTypeDebug", "memTypeCount %u", memTypeCount);
  for (int i = 0; i < static_cast<int>(memTypeCount); ++i)
  {
    // TODO probably bug here with flags.
    auto memType = memPtr[i];
    if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eDeviceLocal))
    {
      F_ILOG("MemoryTypeDebug", "type %u was eDeviceLocal", i);
      if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
      {
        F_ILOG("MemoryTypeDebug", "type %u was eHostVisible", i);
        // weird memory only for uma... usually
        m_memoryTypes.deviceHostIndex = i;
      }
      else
      {
        F_ILOG("MemoryTypeDebug", "type %u was not eHostVisible", i);
        m_memoryTypes.deviceLocalIndex = i;
      }
    }
    else if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
    {
      F_ILOG("MemoryTypeDebug", "type %u was eHostVisible", i);
      if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
      {
        F_ILOG("MemoryTypeDebug", "type %u was eHostCached", i);
        m_memoryTypes.hostCachedIndex = i;
      }
      else
      {
        F_ILOG("MemoryTypeDebug", "type %u was no nott eHostCached", i);
        m_memoryTypes.hostNormalIndex = i;
      }
    }
  }
  // validify memorytypes
  if (m_memoryTypes.deviceHostIndex == 0 || ((m_memoryTypes.deviceLocalIndex != -1) || (m_memoryTypes.hostNormalIndex != -1)))
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
    return std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue*) { m_freeQueueIndexes.dma.push_back(queueId); });
  }
  if (!m_freeQueueIndexes.universal.empty())
  {
    queueFamilyIndex = m_freeQueueIndexes.universalIndex;
    queueId = m_freeQueueIndexes.universal.back();
    m_freeQueueIndexes.universal.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue*) { m_freeQueueIndexes.universal.push_back(queueId); });
  }

  return std::shared_ptr<vk::Queue>(nullptr);
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
    return std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue*) { m_freeQueueIndexes.compute.push_back(queueId); });
  }
  if (!m_freeQueueIndexes.universal.empty())
  {
    queueFamilyIndex = m_freeQueueIndexes.universalIndex;
    queueId = m_freeQueueIndexes.universal.back();
    m_freeQueueIndexes.universal.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId);
    return std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue*) { m_freeQueueIndexes.universal.push_back(queueId); });
  }

  return std::shared_ptr<vk::Queue>(nullptr);
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
    return std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue*) { m_freeQueueIndexes.graphics.push_back(queueId); });
  }
  if (!m_freeQueueIndexes.universal.empty())
  {
    queueFamilyIndex = m_freeQueueIndexes.universalIndex;
    queueId = m_freeQueueIndexes.universal.back();
    m_freeQueueIndexes.universal.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return std::shared_ptr<vk::Queue>(new vk::Queue(que), [&](vk::Queue*) { m_freeQueueIndexes.universal.push_back(queueId); });
  }

  return std::shared_ptr<vk::Queue>(nullptr);
}

VulkanCmdBuffer VulkanGpuDevice::createDMACommandBuffer()
{
  vk::CommandPoolCreateInfo poolInfo;
  if (m_dmaQueues)
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .setQueueFamilyIndex(m_freeQueueIndexes.dmaIndex);
  }
  else
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .setQueueFamilyIndex(m_freeQueueIndexes.universalIndex);
  }
  auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
  auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
    .setCommandBufferCount(1)
    .setCommandPool(pool)
    .setLevel(vk::CommandBufferLevel::ePrimary));
  std::shared_ptr<vk::CommandBuffer> retBuf(new vk::CommandBuffer(buffer[0]));
  std::shared_ptr<vk::CommandPool> retPool(new vk::CommandPool(pool), [&](vk::CommandPool* pool) { m_device->destroyCommandPool(*pool, m_alloc_info); });
  return VulkanCmdBuffer(retBuf, retPool);
}

VulkanCmdBuffer VulkanGpuDevice::createComputeCommandBuffer()
{
  vk::CommandPoolCreateInfo poolInfo;
  if (m_computeQueues)
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .setQueueFamilyIndex(m_freeQueueIndexes.computeIndex);
  }
  else
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .setQueueFamilyIndex(m_freeQueueIndexes.universalIndex);
  }
  auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
  auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
    .setCommandBufferCount(1)
    .setCommandPool(pool)
    .setLevel(vk::CommandBufferLevel::ePrimary));
  std::shared_ptr<vk::CommandBuffer> retBuf(new vk::CommandBuffer(buffer[0]));
  std::shared_ptr<vk::CommandPool> retPool(new vk::CommandPool(pool), [&](vk::CommandPool* pool) { m_device->destroyCommandPool(*pool, m_alloc_info); });
  return VulkanCmdBuffer(retBuf, retPool);
}

VulkanCmdBuffer VulkanGpuDevice::createGraphicsCommandBuffer()
{
  vk::CommandPoolCreateInfo poolInfo;
  if (m_graphicQueues)
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .setQueueFamilyIndex(m_freeQueueIndexes.graphicsIndex);
  }
  else
  {
    poolInfo = vk::CommandPoolCreateInfo()
      .setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient))
      .setQueueFamilyIndex(m_freeQueueIndexes.universalIndex);
  }
  auto pool = m_device->createCommandPool(poolInfo, m_alloc_info);
  auto buffer = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo()
    .setCommandBufferCount(1)
    .setCommandPool(pool)
    .setLevel(vk::CommandBufferLevel::ePrimary));
  std::shared_ptr<vk::CommandBuffer> retBuf(new vk::CommandBuffer(buffer[0]));
  std::shared_ptr<vk::CommandPool> retPool(new vk::CommandPool(pool), [&](vk::CommandPool* pool) { m_device->destroyCommandPool(*pool, m_alloc_info); });
  return VulkanCmdBuffer(retBuf, retPool);
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
    else if ((desc.m_heapType == HeapType::Readback || desc.m_heapType == HeapType::Upload) && m_memoryTypes.hostNormalIndex != -1)
    {
      memoryTypeIndex = m_memoryTypes.hostNormalIndex;
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
    m_device->freeMemory(*memory, m_alloc_info);
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
  m_device->bindBufferMemory(buffer, *memory, offset);
  auto ret = std::shared_ptr<vk::Buffer>(new vk::Buffer(buffer), [&, offset, pagesNeeded, memory](vk::Buffer* buffer)
  {
    heap.freePages(offset, pagesNeeded);
    m_device->destroyBuffer(*buffer, m_alloc_info);
  });

  std::function<RawMapping(int64_t, int64_t)> mapper = [&, memory, usage, offset](int64_t offsetIntoBuffer, int64_t size)
  {
    // insert some mapping asserts here
    F_ASSERT(usage == ResourceUsage::UploadHeap || usage == ResourceUsage::ReadbackHeap, "cannot map device memory");
    RawMapping mapped;
    auto mapping = m_device->mapMemory(*memory, offset + offsetIntoBuffer, size, vk::MemoryMapFlags());
	std::shared_ptr<uint8_t*> target(reinterpret_cast<uint8_t**>(&mapping),
		[&, memory](uint8_t**) -> void
    {
      m_device->unmapMemory(*memory);
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

VulkanPipeline VulkanGpuDevice::createComputePipeline(PipelineLayout, ComputePipelineDescriptor desc)
{

  // create class that can compile shaders for starters.

  vk::ShaderModule readyShader = m_shaders.shader(*m_device, desc.shader(), ShaderStorage::ShaderType::Compute);

  // BindingObject primitive here
  // so I guess I will have
  // few srv's of buffers/textures
  // uav's of buffers/textures
  // samplers
  // lets do it dynamic for starters, maybe bindless later, when vulkan is more mature.

  // one dynamic buffer for constants, big buffer bound with just offset
  vk::DescriptorSetLayoutBinding constantsRingBuffer = vk::DescriptorSetLayoutBinding()
    .setBinding(0)
    .setDescriptorCount(4)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eAll);

  // 6 srvs + uav buffers
  vk::DescriptorSetLayoutBinding srvBuffer = vk::DescriptorSetLayoutBinding()
    .setBinding(1)
    .setDescriptorCount(4)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eAll);

  /*
  vk::DescriptorSetLayoutBinding uavBuffer = vk::DescriptorSetLayoutBinding()
    .setBinding(2)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eAll);
    */
  // todo add textures
  // TODO maybe employ some evil macro plans to have per stage freely customized inputs.
  // How this would fit dx12, not sure. But I guess my priorities are currently vulkan first, dx12 maybe afterwards.
  // Even vulkan as it is, is quite a beast.

  vk::DescriptorSetLayoutBinding bindings[2] = { constantsRingBuffer, srvBuffer };

  vk::DescriptorSetLayoutCreateInfo sampleLayout = vk::DescriptorSetLayoutCreateInfo()
    .setPBindings(bindings)
    .setBindingCount(2);

  vk::DescriptorSetLayout descriptorSetLayout;

  descriptorSetLayout = m_device->createDescriptorSetLayout(sampleLayout);

  auto layoutInfo = vk::PipelineLayoutCreateInfo()
    .setPSetLayouts(&descriptorSetLayout)
    .setSetLayoutCount(1);
  auto layout = m_device->createPipelineLayout(layoutInfo);

  //auto specialiInfo = vk::SpecializationInfo(); // specialisation constant control, not exposed yet by spirv apis
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
  });

  auto pipelineLayout = std::shared_ptr<vk::PipelineLayout>(new vk::PipelineLayout(layout), [&, descriptorSetLayout](vk::PipelineLayout* layout)
  {
    m_device->destroyPipelineLayout(*layout);
    m_device->destroyDescriptorSetLayout(descriptorSetLayout);
  });

  m_device->destroyShaderModule(readyShader);

  return VulkanPipeline(pipeline, pipelineLayout, desc);
}
