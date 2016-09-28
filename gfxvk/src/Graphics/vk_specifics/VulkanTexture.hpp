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

class VulkanTexture
{
  friend class VulkanGpuDevice;

  std::shared_ptr<vk::Image> m_resource;
  ResourceDescriptor m_desc;

  VulkanTexture()
    : m_resource(nullptr)
  {}

  VulkanTexture(std::shared_ptr<vk::Image> impl, ResourceDescriptor desc)
    : m_resource(std::forward<decltype(impl)>(impl))
    , m_desc(std::forward<decltype(desc)>(desc))
  {}
public:
  template<typename T>
  VulkanMappedTexture<T> Map()
  {
    return VulkanMappedTexture<T>(nullptr);
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

using TextureImpl = VulkanTexture;

class VulkanTextureShaderView
{
private:
	friend class VulkanGpuDevice;
	friend class TextureShaderView;
	vk::DescriptorImageInfo m_info;
	vk::DescriptorType m_viewType;
	VulkanTextureShaderView(vk::DescriptorImageInfo info, vk::DescriptorType viewType)
		: m_info(info)
		, m_viewType(viewType)
	{}
public:
	VulkanTextureShaderView()
	{}
	vk::DescriptorImageInfo& info()
	{
		return m_info;
	}
	vk::DescriptorType& type()
	{
		return m_viewType;
	}
};

using TextureShaderViewImpl = VulkanTextureShaderView;

