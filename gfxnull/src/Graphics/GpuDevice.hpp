// GpuDevice.hpp I mean, how else would you name this? Provides compute and graphics.
// Need one for dx12 and vulkan, coexistance? I guess would be fun to compare on windows.
#pragma once

#include "Pipeline.hpp"
#include "PipelineDescriptor.hpp"
#include "CommandQueue.hpp"
#include "CommandList.hpp"
#include "SwapChain.hpp"
#include "Fence.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "GpuResourceViewHeaper.hpp"
#include "core/src/Platform/Window.hpp"
#include "Descriptors/Descriptors.hpp"
#include "DescriptorHeapManager.hpp"
#include "ResourceDescriptor.hpp"

#include <string>

class GpuDevice
{
private:

  void* m_device;
  bool  m_debugLayer;
  DescriptorHeapManager heap;
public:
  GpuDevice(void* device, bool debugLayer);
  ~GpuDevice();
  SwapChain createSwapChain(GraphicsQueue&, Window&);
  SwapChain createSwapChain(GraphicsQueue& queue, Window& wnd, int, FormatType);;
  GpuFence createFence();
  GraphicsQueue createQueue();
  GraphicsCmdBuffer createUniversalCommandList();
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);
  GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);
  Buffer createBuffer(ResourceDescriptor desc);
  Texture createTexture(ResourceDescriptor desc);
  // shader views
  TextureSRV createTextureSRV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureUAV createTextureUAV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureRTV createTextureRTV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureDSV createTextureDSV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  BufferSRV createBufferSRV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferUAV createBufferUAV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferCBV createBufferCBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferIBV createBufferIBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  bool isValid()
  {
    return true;
  }

  // If you want SRGB, https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064.aspx
  // basically create pipeline and pretend that the rtv is SRGB. It will get handled properly.
  SwapChain createSwapChain(GraphicsQueue /*queue*/, Window& /*window*/, unsigned int /*bufferCount = 2*/, FormatType /*type = FormatType::R8G8B8A8_UNORM*/)
  {
     return SwapChain();
  }

  DescriptorHeapManager& getDescHeaps()
  {
    return heap;
  }

  ResourceViewManager& getGenericDescriptorHeap()
  {
    return heap.getGeneric();
  }

};
