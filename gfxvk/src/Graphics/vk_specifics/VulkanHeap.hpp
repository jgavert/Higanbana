#pragma once
#include "gfxvk/src/Graphics/Descriptors/ResUsage.hpp"
#include "gfxvk/src/Graphics/HeapDescriptor.hpp"
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>


struct RawMapping 
{
	std::shared_ptr<uint8_t*> mapped;

  bool isValid()
  {
    return mapped.get() != nullptr;
  }
};

class VulkanMemoryHeap
{
  friend class VulkanGpuDevice;
  friend class ResourceHeap;
  std::shared_ptr<vk::DeviceMemory> m_resource;
  HeapDescriptor m_desc;

  VulkanMemoryHeap()
    : m_resource(nullptr)
  {}
  VulkanMemoryHeap(std::shared_ptr<vk::DeviceMemory> impl, HeapDescriptor desc)
    : m_resource(std::forward<decltype(impl)>(impl))
    , m_desc(std::forward<HeapDescriptor>(desc))
  {}

public:
  HeapDescriptor desc()
  {
    return m_desc;
  }

  bool isValid()
  {
    return true;
  }
};

using MemoryHeapImpl = VulkanMemoryHeap;

