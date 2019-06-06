#pragma once

#include <graphics/common/resources.hpp>
#include <graphics/common/handle.hpp>
#include <graphics/desc/formats.hpp>
#include <core/system/memview.hpp>


namespace faze
{
  namespace backend
  {
    struct DeviceGroupData : std::enable_shared_from_this<DeviceGroupData>
    {
      vector<std::shared_ptr<prototypes::DeviceImpl>> m_impl;
      HandleManager m_handles;
      vector<HeapManager> m_heaps;
      deque<LiveCommandBuffer2> m_buffers;

      DeviceGroupData(vector<std::shared_ptr<prototypes::DeviceImpl>> impl);
      DeviceGroupData(DeviceGroupData&& data) = default;
      DeviceGroupData(const DeviceGroupData& data) = delete;
      DeviceGroupData& operator=(DeviceGroupData&& data) = default;
      DeviceGroupData& operator=(const DeviceGroupData& data) = delete;
      ~DeviceGroupData();

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
      Texture createTexture(ResourceDescriptor desc);

      BufferSRV createBufferSRV(Buffer texture, ShaderViewDescriptor viewDesc);
      BufferUAV createBufferUAV(Buffer texture, ShaderViewDescriptor viewDesc);
      TextureSRV createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureUAV createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureRTV createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureDSV createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc);

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