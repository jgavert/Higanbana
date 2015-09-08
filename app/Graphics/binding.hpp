#pragma once
#include "Buffer.hpp"
#include "Texture.hpp"

class Binding
{
	friend class GfxCommandList;
	friend class ComputeCommandList;

public:
	void UAV(unsigned int index, BufferUAV buf);
	void UAV(unsigned int index, TextureUAV tex);
	void SRV(unsigned int index, BufferSRV buf);
	void SRV(unsigned int index, TextureSRV tex);
};
