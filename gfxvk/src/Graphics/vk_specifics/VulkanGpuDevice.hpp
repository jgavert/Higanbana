#pragma once
#include "VulkanQueue.hpp"
#include "VulkanCmdBuffer.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include <vulkan/vk_cpp.h>

class VulkanGpuDevice
{
private:

  vk::AllocationCallbacks m_alloc_info;
  FazPtrVk<vk::Device>    m_device;
  bool                    m_debugLayer;
  std::vector<vk::QueueFamilyProperties> m_queues;
public:
  VulkanGpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, std::vector<vk::QueueFamilyProperties> queues, bool debugLayer);
  ~VulkanGpuDevice();
  VulkanQueue createDMAQueue();
  VulkanQueue createComputeQueue();
  VulkanQueue createGraphicsQueue();
  VulkanCmdBuffer createDMACommandBuffer();
  VulkanCmdBuffer createComputeCommandBuffer();
  VulkanCmdBuffer createGraphicsCommandBuffer();
  bool isValid();
};

using GpuDeviceImpl = VulkanGpuDevice;
