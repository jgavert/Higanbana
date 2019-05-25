#pragma once

#include <graphics/common/resources.hpp>

namespace faze
{
  namespace backend
  {
    struct DeviceData : std::enable_shared_from_this<DeviceData>
    {
      std::shared_ptr<prototypes::DeviceImpl> m_impl;
      HeapManager m_heaps;

      std::shared_ptr<ResourceTracker<prototypes::BufferImpl>> m_trackerbuffers;
      std::shared_ptr<ResourceTracker<prototypes::TextureImpl>> m_trackertextures;

      std::shared_ptr<std::atomic<int64_t>> m_idGenerator;
      deque<LiveCommandBuffer> m_buffers;

      int64_t newId() { return (*m_idGenerator)++; }

      DeviceData(std::shared_ptr<prototypes::DeviceImpl> impl);
      DeviceData(DeviceData&& data) = default;
      DeviceData(const DeviceData& data) = delete;
      DeviceData& operator=(DeviceData&& data) = default;
      DeviceData& operator=(const DeviceData& data) = delete;
      ~DeviceData();

      void checkCompletedLists();
      void gc();
      void garbageCollection();
      void waitGpuIdle();
      Swapchain createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor);
      void adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor);
      TextureRTV acquirePresentableImage(Swapchain& swapchain);
      TextureRTV* tryAcquirePresentableImage(Swapchain& swapchain);

      Renderpass createRenderpass();
      ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);
      GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);

      Buffer createBuffer(ResourceDescriptor desc);
      SharedBuffer createSharedBuffer(DeviceData& secondary, ResourceDescriptor desc);
      Texture createTexture(ResourceDescriptor desc);
      SharedTexture createSharedTexture(DeviceData& secondary, ResourceDescriptor desc);

      BufferSRV createBufferSRV(Buffer texture, ShaderViewDescriptor viewDesc);
      BufferUAV createBufferUAV(Buffer texture, ShaderViewDescriptor viewDesc);
      TextureSRV createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureUAV createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureRTV createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureDSV createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc);

      GpuSemaphore createSemaphore();
      GpuSharedSemaphore createSharedSemaphore(DeviceData& secondary);

      DynamicBufferView dynamicBuffer(MemView<uint8_t> view, FormatType type);
      DynamicBufferView dynamicBuffer(MemView<uint8_t> view, unsigned stride);
      // streaming

      bool uploadInitialTexture(Texture& tex, CpuImage& image);

      // commandgraph
      void submit(Swapchain& swapchain, CommandGraph graph);
      void submit(CommandGraph graph);
      void explicitSubmit(CommandGraph graph);
      void present(Swapchain& swapchain);
    };
  }
}