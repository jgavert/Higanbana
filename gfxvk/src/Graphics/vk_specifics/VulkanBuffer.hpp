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

struct VulkanBufferState
{
  vk::AccessFlags flags = vk::AccessFlagBits(0);
  int queueIndex = 0;
};

class VulkanBuffer
{
  friend class VulkanGpuDevice;
  friend class VulkanCmdBuffer;
  friend class DependencyTracker;

  std::shared_ptr<vk::Buffer> m_resource;
  std::shared_ptr<VulkanBufferState> m_state;
  int64_t uniqueId = -1;
  unsigned int resourceSize = 0;
  std::function<RawMapping(int64_t, int64_t)> m_mapResource; // on vulkan, heap is here.

  VulkanBuffer()
    : m_resource(nullptr)
  {}

  VulkanBuffer(int64_t uniqueId, std::shared_ptr<vk::Buffer> impl, unsigned resourceSize)
    : m_resource(std::forward<decltype(impl)>(impl))
    , m_state(std::make_shared<VulkanBufferState>())
    , uniqueId(uniqueId)
    , resourceSize(resourceSize)
  {
  }
public:
  template<typename T>
  VulkanMappedBuffer<T> Map(int64_t offsetInBytes, int64_t sizeInBytes)
  {
    return VulkanMappedBuffer<T>{m_mapResource(offsetInBytes, sizeInBytes)};
  }

  bool isValid()
  {
    return m_resource.get() != nullptr;
  }

  vk::Buffer& impl()
  {
    return *m_resource;
  }

  bool operator<(const VulkanBuffer& other) const
  {
    return uniqueId < other.uniqueId;
  }
  bool operator==(const VulkanBuffer& other) const
  {
    return uniqueId == other.uniqueId;
  }
};

using BufferImpl = VulkanBuffer;

class VulkanBufferShaderView
{
private:
  friend class VulkanGpuDevice;
  friend class BufferShaderView;
  friend class DependencyTracker;
  vk::DescriptorBufferInfo m_info;
  vk::DescriptorType m_viewType;
  std::shared_ptr<VulkanBufferState> m_state;

  int64_t uniqueId = -1;
  VulkanBufferShaderView(vk::DescriptorBufferInfo info, vk::DescriptorType viewType, std::shared_ptr<VulkanBufferState> state, int64_t uniqueId)
    : m_info(info)
	  , m_viewType(viewType)
    , m_state(state)
    , uniqueId(uniqueId)
  {}
public:
	VulkanBufferShaderView() {}
	vk::DescriptorBufferInfo& info()
	{
		return m_info;
	}
	vk::DescriptorType& type()
	{
		return m_viewType;
	}
  bool operator<(const VulkanBufferShaderView& other) const
  {
    return uniqueId < other.uniqueId;
  }
  bool operator==(const VulkanBufferShaderView& other) const
  {
    return uniqueId == other.uniqueId;
  }
};

using BufferShaderViewImpl = VulkanBufferShaderView;

