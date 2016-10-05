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

	void read(unsigned slot, BufferSRV& srv)
	{
		set.read(slot, srv.getView());
	}
	void read(unsigned slot, BufferUAV& uav)
	{
		set.read(slot, uav.getView());
	}
	void read(unsigned slot, TextureSRV& srv)
	{
		set.read(slot, srv.getView());
	}
	void read(unsigned slot, TextureUAV& uav)
	{
		set.read(slot, uav.getView());
	}

  void modify(unsigned slot, BufferUAV& uav)
  {
    set.modify(slot, uav.getView());
  }

  void modify(unsigned slot, TextureUAV& uav)
  {
    set.modify(slot, uav.getView());
  }
};