#pragma once
#include "VulkanQueue.hpp"
#include "VulkanCmdBuffer.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include <vulkan/vk_cpp.h>

using GpuDeviceImpl = VulkanGpuDevice;

class VulkanGpuDevice
{
private:

  vk::AllocationCallbacks m_alloc_info;
  FazPtrVk<vk::Device>    m_device;
  bool                    m_debugLayer;
public:
  VulkanGpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, bool debugLayer);
  ~VulkanGpuDevice();
  VulkanQueue createGraphicsQueue();
  VulkanCmdBuffer createGraphicsCommandBuffer();
  bool isValid();
};
