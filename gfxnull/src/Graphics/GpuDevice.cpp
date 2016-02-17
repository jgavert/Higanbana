#include "GpuDevice.hpp"

GpuDevice::GpuDevice(void* device, bool debugLayer) : m_device(device), m_debugLayer(debugLayer)
{
}

GpuDevice::~GpuDevice()
{
}


// Needs to be created from descriptor
GpuFence GpuDevice::createFence()
{
  return GpuFence();
}

GpuCommandQueue GpuDevice::createQueue()
{
  return GpuCommandQueue(nullptr);
}

// Needs to be created from descriptor
GfxCommandList GpuDevice::createUniversalCommandList()
{
  return GfxCommandList(nullptr);
}

ComputePipeline GpuDevice::createComputePipeline(ComputePipelineDescriptor )
{
	return ComputePipeline();
}

GraphicsPipeline GpuDevice::createGraphicsPipeline(GraphicsPipelineDescriptor )
{
	return GraphicsPipeline();
}

Buffer_new GpuDevice::createBuffer(ResourceDescriptor resDesc)
{
  Buffer_new buf = {};
  buf.m_desc = resDesc;
  return buf;
}

Texture_new GpuDevice::createTexture(ResourceDescriptor resDesc)
{
  Texture_new tex = {};
  tex.m_desc = resDesc;
  return tex;
}