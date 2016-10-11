#pragma once
#include "vk_specifics/VulkanGpuDevice.hpp"
#include "vk_specifics/VulkanQueue.hpp"
#include "CommandList.hpp"
#include "Pipeline.hpp"
#include "PipelineDescriptor.hpp"
#include "ResourceDescriptor.hpp"
#include "HeapDescriptor.hpp"
#include "Heap.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"
#include "Fence.hpp"
#include "Swapchain.hpp"
#include "WindowSurface.hpp"

#include "core/src/system/SequenceTracker.hpp"
#include "core/src/system/SequenceRingRangeAllocator.hpp"

#include <deque>
#include <vector>

class GpuDevice
{
private:
  friend class GraphicsInstance;
  std::shared_ptr<GpuDeviceImpl> m_device;
  faze::SequenceTracker m_tracker;

  struct LiveCmdBuffer
  {
    FenceImpl fence;
    GraphicsCmdBuffer cmdBuffer;
  };
  std::deque<LiveCmdBuffer> m_liveCmdBuffers;

  QueueImpl m_queue;
  std::vector<std::shared_ptr<CmdBufferImpl>> m_rawCommandBuffers;
  std::vector<DescriptorPool> m_descriptorPools;
  faze::SequenceRingRangeAllocator m_cmdBufferAllocator;


  std::vector<FenceImpl> m_rawFences;
  faze::SequenceRingRangeAllocator m_fenceAllocator;


  GpuDevice(GpuDeviceImpl device);
  void updateCompletedSequences();
  void destroyResources();
public:
  ~GpuDevice();

  bool isValid();
  GraphicsCmdBuffer createGraphicsCommandBuffer();
  GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);

  std::vector<ResourceDescriptor> querySwapChainInfo(WindowSurface& surface);
  std::vector<PresentMode> queryPresentModes(WindowSurface& surface);
  Swapchain createSwapchain(WindowSurface& surface, PresentMode mode, ResourceDescriptor chosen = ResourceDescriptor().setFormat(FormatType::Unknown));

  template <typename ShaderType>
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc)
  {
    return m_device->createComputePipeline<ShaderType>(desc);
  }

  ResourceHeap createMemoryHeap(HeapDescriptor desc);
  Buffer createBuffer(ResourceHeap& heap, ResourceDescriptor desc);
  Texture createTexture(ResourceHeap& heap, ResourceDescriptor desc);

  TextureSRV createTextureSRV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureUAV createTextureUAV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureRTV createTextureRTV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureDSV createTextureDSV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  BufferSRV createBufferSRV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferUAV createBufferUAV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferCBV createBufferCBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferIBV createBufferIBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  // all queue related
  void submit(GraphicsCmdBuffer& gfx);
  bool fenceDone(Fence fence);
  void waitFence(Fence fence);
  void waitIdle();

  // present

  TextureRTV acquirePresentableImage(Swapchain& swapchain);
  void submitSwapchain(GraphicsCmdBuffer& gfx, Swapchain& sc);
  void present(Swapchain& swapchain);
};
