#pragma once
#include "gfxvk/src/Graphics/Descriptors/ResUsage.hpp"
#include "gfxvk/src/Graphics/ResourceDescriptor.hpp"
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>

template<typename type>
struct VulkanMappedTexture
{
  std::shared_ptr<vk::Image> m_mappedresource;
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
using MappedTextureImpl = VulkanMappedTexture<T>;

struct VulkanImageState
{
  vk::AccessFlags flags = vk::AccessFlagBits(0);
  vk::ImageLayout layout = vk::ImageLayout::eUndefined;
  int queueIndex = 0;
};

class VulkanTexture
{
  friend class VulkanGpuDevice;

  std::shared_ptr<vk::Image> m_resource;
  std::shared_ptr<VulkanImageState> m_state;
  int64_t uniqueId = -1;

  VulkanTexture()
    : m_resource(nullptr)
  {}

  VulkanTexture(int64_t uniqueId, std::shared_ptr<vk::Image> impl)
    : m_resource(std::forward<decltype(impl)>(impl))
    , m_state(std::make_shared<VulkanImageState>())
    , uniqueId(uniqueId)
  {}
public:
  template<typename T>
  VulkanMappedTexture<T> Map()
  {
    return VulkanMappedTexture<T>(nullptr);
  }

  vk::Image& impl()
  {
    return *m_resource;
  }

  bool isValid()
  {
    return m_resource.get() != nullptr;
  }

  bool operator<(const VulkanTexture& other) const
  {
    return uniqueId < other.uniqueId;
  }
  bool operator==(const VulkanTexture& other) const
  {
    return uniqueId == other.uniqueId;
  }
};

using TextureImpl = VulkanTexture;

class VulkanTextureShaderView
{
private:
	friend class VulkanGpuDevice;
	friend class TextureShaderView;
	vk::DescriptorImageInfo m_info;
	vk::DescriptorType m_viewType;
  std::shared_ptr<VulkanImageState> m_state;

  int64_t uniqueId = -1;

	VulkanTextureShaderView(vk::DescriptorImageInfo info, vk::DescriptorType viewType, std::shared_ptr<VulkanImageState> state, int64_t uniqueId)
		: m_info(info)
		, m_viewType(viewType)
    , m_state(state)
    , uniqueId(uniqueId)
	{}
public:
	VulkanTextureShaderView()	{}
	vk::DescriptorImageInfo& info()
	{
		return m_info;
	}
	vk::DescriptorType& type()
	{
		return m_viewType;
	}
  bool operator<(const VulkanTextureShaderView& other) const
  {
    return uniqueId < other.uniqueId;
  }
  bool operator==(const VulkanTextureShaderView& other) const
  {
    return uniqueId == other.uniqueId;
  }
};

using TextureShaderViewImpl = VulkanTextureShaderView;

