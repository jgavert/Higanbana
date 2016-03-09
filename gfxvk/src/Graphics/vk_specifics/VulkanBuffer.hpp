#pragma once
#include "gfxvk/src/Graphics/Descriptors/ResUsage.hpp"
#include "gfxvk/src/Graphics/ResourceDescriptor.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include <vector>
#include <memory>
#include <vulkan/vk_cpp.h>

template<typename type>
struct VulkanMappedBuffer
{
  FazPtrVk<vk::Buffer> m_mappedresource;
  type mapped;
  VulkanMappedBuffer(FazPtrVk<vk::Buffer> res)
    : m_mappedresource(std::forward<decltype(res)>(res))
  {}
  ~VulkanMappedBuffer()
  {
  }

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
    return false;
  }
};

template <typename T>
using MappedBufferImpl = VulkanMappedBuffer<T>;

class VulkanBuffer
{
  FazPtrVk<vk::Buffer> m_resource;
  ResourceDescriptor m_desc;
public:
  template<typename T>
  VulkanMappedBuffer<T> Map()
  {
    return VulkanMappedBuffer<T>(nullptr);
  }

  bool isValid()
  {
    return true;
  }

  ResourceDescriptor& desc()
  {
    return m_desc;
  }
};

using BufferImpl = VulkanBuffer;

class VulkanBufferShaderView
{
private:
  FazPtr<size_t> indexInHeap; // will handle removing references from heaps when destructed. ref counted.
  size_t customIndex;
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

