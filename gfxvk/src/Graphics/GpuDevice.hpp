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
  FazPtrVk<vk::Device> m_device;
  bool  m_debugLayer;
  DescriptorHeapManager heap;
public:
  GpuDevice(FazPtrVk<vk::Device> device, vk::AllocationCallbacks alloc_info, bool debugLayer);
  ~GpuDevice();
  SwapChain createSwapChain(GpuCommandQueue&, Window&);
  SwapChain createSwapChain(GpuCommandQueue& queue, Window& wnd, int, FormatType);
  GpuFence createFence();
  GpuCommandQueue createQueue();
  GfxCommandList createUniversalCommandList();
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);
  GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);
  Buffer_new createBuffer(ResourceDescriptor desc);
  Texture_new createTexture(ResourceDescriptor desc);

  bool isValid()
  {
    return m_device.isValid();
  }

public:
  template <typename ...Args>
  BufferSRV createBufferSRV(Args&& ... )
  {
    return BufferSRV();
  }

  template <typename ...Args>
  BufferUAV createBufferUAV(Args&& ... )
  {
    return BufferUAV();
  }

  template <typename ...Args>
  BufferIBV createBufferIBV(Args&& ... )
  {
    return BufferIBV();
  }

  template <typename ...Args>
  BufferCBV createConstantBuffer(Args&& ... )
  {
    return BufferCBV();
  }


  public:

	  template <typename ...Args>
	  TextureSRV createTextureSrvObj(Args&& ... )
	  {
      return TextureSRV();
	  }




  template <typename ...Args>
  TextureSRV createTextureSRV(Args&& ... )
  {
    return TextureSRV();
  }

  template <typename ...Args>
  TextureUAV createTextureUavObj(Args&& ... )
  {
    return TextureUAV();
  }

  template <typename ...Args>
  TextureUAV createTextureUAV(Args&& ... )
  {
    return TextureUAV();
  }

  template <typename ...Args>
  TextureRTV createTextureRTV(Args&& ... )
  {
    return TextureRTV();
  }
  template <typename ...Args>
  TextureDSV createTextureDSV(Args&& ... )
  {
    return TextureDSV();
  }

  // If you want SRGB, https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064.aspx
  // basically create pipeline and pretend that the rtv is SRGB. It will get handled properly.
  SwapChain createSwapChain(GpuCommandQueue /*queue*/, Window& /*window*/, unsigned int /*bufferCount = 2*/, FormatType /*type = FormatType::R8G8B8A8_UNORM*/)
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
