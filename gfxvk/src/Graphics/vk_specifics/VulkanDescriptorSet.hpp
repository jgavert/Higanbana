#pragma once
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanPipeline.hpp"

#include <utility>

class VulkanDescriptorSet
{
private:
	friend class VulkanGpuDevice;
	friend class VulkanCmdBuffer;
  friend class VulkanBindingInformation;
	//vk::DescriptorSet set; // this goes to Commandlist
  vk::PipelineLayout layout;
  vk::DescriptorSetLayout descriptorLayout;
	std::vector<std::pair< unsigned, VulkanBufferShaderView >> buffers;
	std::vector<std::pair< unsigned, VulkanTextureShaderView>> textures;
public:

	VulkanDescriptorSet(VulkanPipeline& pipeline)
		: layout(*pipeline.m_pipelineLayout)
    , descriptorLayout(*pipeline.m_descriptorSetLayout)
	{}

	VulkanDescriptorSet(vk::PipelineLayout layout, vk::DescriptorSetLayout descriptorLayout)
		: layout(layout)
    , descriptorLayout(descriptorLayout)
	{}

	void read(unsigned slot, VulkanBufferShaderView& buffer);
	void read(unsigned slot, VulkanTextureShaderView& texture);
	void modify(unsigned slot, VulkanBufferShaderView& buffer);
	void modify(unsigned slot, VulkanTextureShaderView& texture);
};

using DescriptorSetImpl = VulkanDescriptorSet;