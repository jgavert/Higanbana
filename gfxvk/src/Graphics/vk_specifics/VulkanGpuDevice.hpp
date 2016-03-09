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
  bool                    m_singleQueue;
  bool                    m_onlySeparateQueues;
  FazPtrVk<vk::Queue>     m_internalUniversalQueue;
  struct FreeQueues
  {
    int universalIndex;
    int graphicsIndex;
    int computeIndex;
    int dmaIndex;
    std::vector<uint32_t> universal;
    std::vector<uint32_t> graphics;
    std::vector<uint32_t> compute;
    std::vector<uint32_t> dma;
  } m_freeQueueIndexes;

public:
  VulkanGpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, std::vector<vk::QueueFamilyProperties> queues, bool debugLayer);
  VulkanQueue createDMAQueue();
  VulkanQueue createComputeQueue();
  VulkanQueue createGraphicsQueue();
  VulkanCmdBuffer createDMACommandBuffer();
  VulkanCmdBuffer createComputeCommandBuffer();
  VulkanCmdBuffer createGraphicsCommandBuffer();
  bool isValid();
};

using GpuDeviceImpl = VulkanGpuDevice;
