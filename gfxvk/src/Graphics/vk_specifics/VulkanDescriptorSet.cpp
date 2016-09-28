#include "VulkanDescriptorSet.hpp"

void VulkanDescriptorSet::bind(unsigned slot, VulkanBufferShaderView& buffer)
{
	buffers.push_back(std::make_pair(slot, buffer));
}
void VulkanDescriptorSet::bind(unsigned slot, VulkanTextureShaderView& texture)
{
	textures.push_back(std::make_pair(slot, texture));
}
std::vector<vk::WriteDescriptorSet> VulkanDescriptorSet::compile()
{
	std::vector<vk::WriteDescriptorSet> allSets;
	for (auto&& it : buffers)
	{
		allSets.push_back(vk::WriteDescriptorSet()
			.setDescriptorCount(1)
			.setDescriptorType(it.second.type())
			.setDstBinding(it.first)
			.setDstSet(set)
			.setPBufferInfo(&it.second.info()));
	}

	return allSets;
}
