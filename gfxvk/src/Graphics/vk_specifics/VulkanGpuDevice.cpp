#include "VulkanGpuDevice.hpp"

VulkanGpuDevice::VulkanGpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, bool debugLayer)
  : m_alloc_info(alloc_info)
  , m_device(device)
  , m_debugLayer(debugLayer)
{
}

VulkanGpuDevice::~VulkanGpuDevice()
{
}

VulkanQueue VulkanGpuDevice::createGraphicsQueue()
{
  FazPtrVk<vk::Queue> ret = FazPtrVk<vk::Queue>([](vk::Queue ) {});
  return VulkanQueue(ret);
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
