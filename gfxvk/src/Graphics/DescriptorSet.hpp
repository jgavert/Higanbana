#pragma once

#include "Buffer.hpp"
#include "Texture.hpp"
#include "vk_specifics/VulkanDescriptorSet.hpp"

#include <memory>

class DescriptorSet
{
private:
	friend class GraphicsCmdBuffer;
	DescriptorSetImpl set;
public:
	DescriptorSet(DescriptorSetImpl& set)
	: set(set)
	{}

	// could change slot to a compile time.

	void bind(unsigned slot, BufferSRV& srv)
	{
		set.bind(slot, srv.getView());
	}
	void bind(unsigned slot, BufferUAV& uav)
	{
		set.bind(slot, uav.getView());
	}
	void bind(unsigned slot, TextureSRV& srv)
	{
		set.bind(slot, srv.getView());
	}
	void bind(unsigned slot, TextureUAV& uav)
	{
		set.bind(slot, uav.getView());
	}
};