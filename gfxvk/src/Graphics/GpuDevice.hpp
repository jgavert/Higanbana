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
#include "core/src/memory/ManagedResource.hpp"

#include <string>
#include <vulkan/vk_cpp.h>

class GpuDevice
{
private:

  vk::AllocationCallbacks m_alloc_info;
  FazPtrVk<vk::Device>    m_device;
  bool                    m_debugLayer;
  DescriptorHeapManager   heap;
public:
  GpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, bool debugLayer);
  ~GpuDevice();
  SwapChain createSwapChain(GraphicsQueue&, Window&);
  SwapChain createSwapChain(GraphicsQueue& queue, Window& wnd, int, FormatType);
  GpuFence createFence();
  GraphicsQueue createQueue();
  GfxCommandList createUniversalCommandList();
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);
  GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);
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
  bool isValid();
  SwapChain createSwapChain(
    GraphicsQueue /*queue*/, Window& /*window*/,
    unsigned int /*bufferCount = 2*/, FormatType /*type = FormatType::R8G8B8A8_UNORM*/);
  DescriptorHeapManager& getDescHeaps();
  ResourceViewManager& getGenericDescriptorHeap();

};
