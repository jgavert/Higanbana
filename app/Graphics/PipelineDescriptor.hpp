#pragma once
#include <d3d12.h>
#include <string>

class ComputePipelineDescriptor
{
	friend class GpuDevice;

  std::string shaderSourcePath;
public:
	ComputePipelineDescriptor()
	{

	}

  ComputePipelineDescriptor& shader(std::string path)
  {
    shaderSourcePath = path;
    return *this;
  }

};

class GfxPipelineDescriptor
{


};
