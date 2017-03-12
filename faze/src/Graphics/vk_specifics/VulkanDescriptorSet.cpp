#include "VulkanDescriptorSet.hpp"

void VulkanDescriptorSet::read(unsigned slot, VulkanBufferShaderView& buffer)
{
  readBuffers.push_back(std::make_pair(slot, buffer));
}
void VulkanDescriptorSet::read(unsigned slot, VulkanTextureShaderView& texture)
{
  readTextures.push_back(std::make_pair(slot, texture));
}
void VulkanDescriptorSet::modify(unsigned slot, VulkanBufferShaderView& buffer)
{
	modifyBuffers.push_back(std::make_pair(slot, buffer));
}
void VulkanDescriptorSet::modify(unsigned slot, VulkanTextureShaderView& texture)
{
	modifyTextures.push_back(std::make_pair(slot, texture));
}