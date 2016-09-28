#pragma once
#include "gfxvk/src/Graphics/Descriptors/ResUsage.hpp"
#include "gfxvk/src/Graphics/ResourceDescriptor.hpp"
#include "VulkanHeap.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <vulkan/vulkan.hpp>

template<typename type>
struct VulkanMappedBuffer
{
  RawMapping m_mapped;

  size_t rangeBegin()
  {
    return 0;
  }

  size_t rangeEnd()
  {
    return 0;
  }

  type& operator[](size_t i)
  {
    return reinterpret_cast<type*>(m_mapped.mapped.get())[i];
  }

  type* get()
  {
    return reinterpret_cast<type*>(m_mapped.mapped.get());
  }

  bool isValid()
  {
    return m_mapped.isValid();
  }
};


template <typename T>
using MappedBufferImpl = VulkanMappedBuffer<T>;

class VulkanBuffer
{
  friend class VulkanGpuDevice;
  friend class VulkanCmdBuffer;

  std::shared_ptr<vk::Buffer> m_resource;
  vk::AccessFlags m_flags;
  ResourceDescriptor m_desc;
  std::function<RawMapping(int64_t, int64_t)> m_mapResource; // on vulkan, heap is here.

  VulkanBuffer()
    : m_resource(nullptr)
  {}

  VulkanBuffer(std::shared_ptr<vk::Buffer> impl, vk::AccessFlags flags, ResourceDescriptor desc)
    : m_resource(std::forward<decltype(impl)>(impl))
    , m_flags(flags)
    , m_desc(std::forward<decltype(desc)>(desc))
  {}
public:
  template<typename T>
  VulkanMappedBuffer<T> Map(int64_t offsetInBytes, int64_t sizeInBytes)
  {
    return VulkanMappedBuffer<T>{m_mapResource(offsetInBytes, sizeInBytes)};
  }

  ResourceDescriptor& desc()
  {
    return m_desc;
  }

  bool isValid()
  {
    return m_resource.get() != nullptr;
  }
};

using BufferImpl = VulkanBuffer;

class VulkanBufferShaderView
{
private:
  friend class VulkanGpuDevice;
  friend class BufferShaderView;
  vk::DescriptorBufferInfo info;
  VulkanBufferShaderView(vk::DescriptorBufferInfo info)
    : info(info)
  {}
public:
	VulkanBufferShaderView() {}
  vk::DescriptorBufferInfo& getImpl()
  {
	  return info;
  }
};

using BufferShaderViewImpl = VulkanBufferShaderView;

