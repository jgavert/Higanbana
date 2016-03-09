#include "VulkanGpuDevice.hpp"

VulkanGpuDevice::VulkanGpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, std::vector<vk::QueueFamilyProperties> queues, bool debugLayer)
  : m_alloc_info(alloc_info)
  , m_device(device)
  , m_debugLayer(debugLayer)
  , m_queues(queues)
  , m_singleQueue(false)
  , m_onlySeparateQueues(false)
  , m_freeQueueIndexes({})
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
    if (current & (GfxQ | CpQ | DMAQ))
    {
      for (uint32_t i = 0; i < it.queueCount(); ++i)
      {
        m_freeQueueIndexes.universal.push_back(i);
      }
      m_freeQueueIndexes.universalIndex = k;
    }
    else if (current & (GfxQ | CpQ))
    {
      for (uint32_t i = 0; i < it.queueCount(); ++i)
      {
        m_freeQueueIndexes.graphics.push_back(i);
      }
      m_freeQueueIndexes.graphicsIndex = k;
    }
    else if (current & CpQ)
    {
      for (uint32_t i = 0; i < it.queueCount(); ++i)
      {
        m_freeQueueIndexes.compute.push_back(i);
      }
      m_freeQueueIndexes.computeIndex = k;
    }
    else if (current & DMAQ)
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
    abort(); // wtf, not sane device.
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
    abort(); // abort mission. Too many variations of queues.
  }
  if (m_freeQueueIndexes.universal.size() == 0
    && m_freeQueueIndexes.graphics.size() > 0
    && m_freeQueueIndexes.compute.size() > 0
    && m_freeQueueIndexes.dma.size() > 0)
  {
    m_onlySeparateQueues = true; // This is ideal for design. Only on amd.
  }
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
  else if (m_onlySeparateQueues && m_freeQueueIndexes.dma.size() > 0)
  {
    // yay, realdeal
    queueFamilyIndex = m_freeQueueIndexes.dmaIndex;
    queueId = m_freeQueueIndexes.dma.back();
    m_freeQueueIndexes.dma.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.dma.push_back(queueId); });
  }
  if (m_freeQueueIndexes.universal.size() > 0)
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
  else if (m_onlySeparateQueues && m_freeQueueIndexes.compute.size() > 0)
  {
    // yay, realdeal
    queueFamilyIndex = m_freeQueueIndexes.computeIndex;
    queueId = m_freeQueueIndexes.compute.back();
    m_freeQueueIndexes.compute.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.compute.push_back(queueId); });
  }
  if (m_freeQueueIndexes.universal.size() > 0)
  {
    queueFamilyIndex = m_freeQueueIndexes.universalIndex;
    queueId = m_freeQueueIndexes.universal.back();
    m_freeQueueIndexes.universal.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
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
  else if (m_onlySeparateQueues && m_freeQueueIndexes.graphics.size() > 0)
  {
    // yay, realdeal
    queueFamilyIndex = m_freeQueueIndexes.graphicsIndex;
    queueId = m_freeQueueIndexes.graphics.back();
    m_freeQueueIndexes.graphics.pop_back();
    auto que = m_device->getQueue(queueFamilyIndex, queueId); // TODO: 0 index is wrong.
    return FazPtrVk<vk::Queue>(que, [&](vk::Queue) { m_freeQueueIndexes.graphics.push_back(queueId); });
  }
  if (m_freeQueueIndexes.universal.size() > 0)
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
  FazPtrVk<vk::CommandBuffer> ret = FazPtrVk<vk::CommandBuffer>([](vk::CommandBuffer) {});
  return VulkanCmdBuffer(ret);
}

VulkanCmdBuffer VulkanGpuDevice::createComputeCommandBuffer()
{
  FazPtrVk<vk::CommandBuffer> ret = FazPtrVk<vk::CommandBuffer>([](vk::CommandBuffer) {});
  return VulkanCmdBuffer(ret);
}

VulkanCmdBuffer VulkanGpuDevice::createGraphicsCommandBuffer()
{
  FazPtrVk<vk::CommandBuffer> ret = FazPtrVk<vk::CommandBuffer>([](vk::CommandBuffer) {});
  return VulkanCmdBuffer(ret);
}

bool VulkanGpuDevice::isValid()
{
  return m_device.isValid();
}
