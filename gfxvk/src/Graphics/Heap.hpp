#pragma once
#include "vk_specifics/VulkanHeap.hpp"
#include "HeapDescriptor.hpp"

class ResourceHeap 
{
  friend class GpuDevice;
  MemoryHeapImpl m_resource;

  ResourceHeap(MemoryHeapImpl impl)
    : m_resource(std::forward<decltype(impl)>(impl))
  {}
public:

  HeapDescriptor desc()
  {
    return m_resource.desc();
  }

  bool isValid()
  {
    return true;
  }
};


