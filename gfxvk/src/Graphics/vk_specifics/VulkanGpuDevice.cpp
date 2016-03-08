#include "VulkanGpuDevice.hpp"

VulkanGpuDevice::VulkanGpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, std::vector<vk::QueueFamilyProperties> queues, bool debugLayer)
  : m_alloc_info(alloc_info)
  , m_device(device)
  , m_debugLayer(debugLayer)
  , m_queues(queues)
{
}

VulkanGpuDevice::~VulkanGpuDevice()
{
}

VulkanQueue VulkanGpuDevice::createDMAQueue()
{
  uint32_t queueIndex = 0;
  for (size_t i = 0; i < m_queues.size(); ++i)
  {
    if ((static_cast<int>(m_queues[i].queueFlags()) & (1 << static_cast<int>(VK_QUEUE_TRANSFER_BIT))) != 0)
    {
      queueIndex = static_cast<uint32_t>(i);
      break;
    }
  }
  auto que = m_device->getQueue(queueIndex, 0); // TODO: 0 index is wrong.
  FazPtrVk<vk::Queue> ret = FazPtrVk<vk::Queue>(que, [](vk::Queue) {}, false);
  return VulkanQueue(ret);
}

VulkanQueue VulkanGpuDevice::createComputeQueue()
{
  uint32_t queueIndex = 0;
  for (size_t i = 0; i < m_queues.size(); ++i)
  {
    if ((static_cast<int>(m_queues[i].queueFlags()) & (1 << static_cast<int>(VK_QUEUE_COMPUTE_BIT))) != 0)
    {
      queueIndex = static_cast<uint32_t>(i);
      break;
    }
  }
  auto que = m_device->getQueue(queueIndex, 0); // TODO: 0 index is wrong.
  FazPtrVk<vk::Queue> ret = FazPtrVk<vk::Queue>(que, [](vk::Queue) {}, false);
  return VulkanQueue(ret);
}

VulkanQueue VulkanGpuDevice::createGraphicsQueue()
{
  uint32_t queueIndex = 0;
  for (size_t i = 0; i < m_queues.size(); ++i)
  {
    if ((static_cast<int>(m_queues[i].queueFlags()) & (1 << static_cast<int>(VK_QUEUE_GRAPHICS_BIT))) != 0)
    {
      queueIndex = static_cast<uint32_t>(i);
      break;
    }
  }
  auto que = m_device->getQueue(queueIndex, 0); // TODO: 0 index is wrong.
  FazPtrVk<vk::Queue> ret = FazPtrVk<vk::Queue>(que, [](vk::Queue) {}, false);
  return VulkanQueue(ret);
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
