#pragma once
#include "vk_specifics/VulkanGpuDevice.hpp"
#include "CommandList.hpp"
#include "CommandQueue.hpp"
#include "Pipeline.hpp"
#include "PipelineDescriptor.hpp"
#include "ResourceDescriptor.hpp"
#include "HeapDescriptor.hpp"
#include "Heap.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"

class GpuDevice
{
private:
  friend class GraphicsInstance;
  GpuDeviceImpl m_device;
  GpuDevice(GpuDeviceImpl device);
public:
  bool isValid();
  DMAQueue createDMAQueue();
  ComputeQueue createComputeQueue();
  GraphicsQueue createGraphicsQueue();
  DMACmdBuffer createDMACommandBuffer();
  ComputeCmdBuffer createComputeCommandBuffer();
  GraphicsCmdBuffer createGraphicsCommandBuffer();
  GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);

  ResourceHeap createMemoryHeap(HeapDescriptor desc);
  Buffer createBuffer(ResourceDescriptor desc);
  Texture createTexture(ResourceDescriptor desc);

  TextureSRV createTextureSRV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureUAV createTextureUAV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureRTV createTextureRTV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureDSV createTextureDSV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  BufferSRV createBufferSRV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferUAV createBufferUAV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferCBV createBufferCBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferIBV createBufferIBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

};
