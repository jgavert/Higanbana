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
  int workGroupX;
  int workGroupY;
  int workGroupZ;
public:
	DescriptorSet(DescriptorSetImpl& set, int x, int y, int z)
	: set(set)
  , workGroupX(x)
  , workGroupY(y)
  , workGroupZ(z)
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