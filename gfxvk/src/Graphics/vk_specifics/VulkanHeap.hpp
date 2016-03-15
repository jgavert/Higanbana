#pragma once
#include "gfxvk/src/Graphics/Descriptors/ResUsage.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include "gfxvk/src/Graphics/HeapDescriptor.hpp"
#include <vector>
#include <memory>
#include <vulkan/vk_cpp.h>


struct RawMapping 
{
  char* mapped;

  size_t rangeBegin()
  {
    return 0;
  }

  size_t rangeEnd()
  {
    return 0;
  }

  bool isValid()
  {
    return mapped != nullptr;
  }
};

class VulkanMemoryHeap
{
  friend class VulkanGpuDevice;
  friend class ResourceHeap;
  FazPtrVk<vk::DeviceMemory> m_resource;
  HeapDescriptor m_desc;

  VulkanMemoryHeap()
    : m_resource(nullptr)
  {}
  VulkanMemoryHeap(FazPtrVk<vk::DeviceMemory> impl, HeapDescriptor desc)
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

