#pragma once
#include "gfxvk/src/Graphics/Descriptors/ResUsage.hpp"
#include "gfxvk/src/Graphics/ResourceDescriptor.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include "VulkanHeap.hpp"
#include <vector>
#include <memory>
#include <vulkan/vk_cpp.h>

template<typename type>
struct VulkanMappedBuffer
{
  FazPtrVk<vk::Buffer> m_mappedresource;
  type mapped;

  size_t rangeBegin()
  {
    return 0;
  }

  size_t rangeEnd()
  {
    return 0;
  }

  type& operator[](size_t)
  {
    return mapped;
  }

  type* get()
  {
    return &mapped;
  }

  bool isValid()
  {
    return mapped != nullptr;
  }
};


template <typename T>
using MappedBufferImpl = VulkanMappedBuffer<T>;

class VulkanBuffer
{
  friend class VulkanGpuDevice;

  FazPtrVk<vk::Buffer> m_resource;
  ResourceDescriptor m_desc;
  std::function<RawMapping(int64_t, int64_t)> m_mapResource; // on vulkan, heap is here.

  VulkanBuffer()
    : m_resource(nullptr)
  {}

  VulkanBuffer(FazPtrVk<vk::Buffer> impl, ResourceDescriptor desc)
    : m_resource(std::forward<decltype(impl)>(impl))
    , m_desc(std::forward<decltype(desc)>(desc))
  {}
public:
  template<typename T>
  VulkanMappedBuffer<T> Map()
  {
    return VulkanMappedBuffer<T>(nullptr);
  }

  ResourceDescriptor& desc()
  {
    return m_desc;
  }

  bool isValid()
  {
    return m_resource.isValid();
  }
};

using BufferImpl = VulkanBuffer;

class VulkanBufferShaderView
{
private:
  friend class VulkanGpuDevice;
  friend class BufferShaderView;

  FazPtr<size_t> indexInHeap; // will handle removing references from heaps when destructed. ref counted.
  size_t customIndex;
  VulkanBufferShaderView()
    : indexInHeap([](size_t) {})
    , customIndex(0)
  {}
public:
  bool isValid()
  {
    return true;
  }

  size_t getIndexInHeap()
  {
    return *indexInHeap.get(); // This is really confusing getter, for completely wrong reasons.
  }

  unsigned getCustomIndexInHeap() // this returns implementation specific index. There might be better ways to do this.
  {
    return static_cast<unsigned>(customIndex);
  }
};

using BufferShaderViewImpl = VulkanBufferShaderView;

