#include "VulkanDescriptorSet.hpp"

void VulkanDescriptorSet::bind(unsigned slot, VulkanBufferShaderView& buffer)
{
	buffers.push_back(std::make_pair(slot, buffer));
}
void VulkanDescriptorSet::bind(unsigned slot, VulkanTextureShaderView& texture)
{
	textures.push_back(std::make_pair(slot, texture));
}
vk::WriteDescriptorSet VulkanDescriptorSet::compile()
{
	vk::WriteDescriptorSet write = vk::WriteDescriptorSet();

	return write;
}
