#pragma once

#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "swapchain.hpp"
#include "commandgraph.hpp"
#include "commandlist.hpp"

namespace faze
{
  class GpuDevice : private backend::SharedState<backend::DeviceData>
  {
  public:
    GpuDevice() = default;
    GpuDevice(backend::DeviceData data)
    {
      makeState(std::forward<decltype(data)>(data));
    }
    GpuDevice(StatePtr state)
    {
      m_state = std::move(state);
    }

    StatePtr state()
    {
      return m_state;
    }
    Buffer createBuffer(ResourceDescriptor descriptor)
    {
      auto buf = S().createBuffer(descriptor);
      return buf;
    }

    Texture createTexture(ResourceDescriptor descriptor)
    {
      auto tex = S().createTexture(descriptor);
      return tex;
    }

    BufferSRV createBufferSRV(Buffer texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createBufferSRV(texture, viewDesc);
    }

    BufferSRV createBufferSRV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createBufferSRV(createBuffer(descriptor), viewDesc);
    }

    BufferUAV createBufferUAV(Buffer texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createBufferUAV(texture, viewDesc);
    }

    BufferUAV createBufferUAV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createBufferUAV(createBuffer(descriptor), viewDesc);
    }

    TextureSRV createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureSRV(texture, viewDesc);
    }

    TextureSRV createTextureSRV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureSRV(createTexture(descriptor), viewDesc);
    }

    TextureUAV createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureUAV(texture, viewDesc);
    }

    TextureUAV createTextureUAV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureUAV(createTexture(descriptor), viewDesc);
    }

    TextureRTV createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureRTV(texture, viewDesc);
    }

    TextureRTV createTextureRTV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureRTV(createTexture(descriptor), viewDesc);
    }

    TextureDSV createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureDSV(texture, viewDesc);
    }

    TextureDSV createTextureDSV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureDSV(createTexture(descriptor), viewDesc);
    }

    template <typename Type>
    DynamicBufferView dynamicBuffer(MemView<Type> view, FormatType type)
    {
      return S().dynamicBuffer(reinterpret_memView<uint8_t>(view), type);
    }

    template <typename Type>
    DynamicBufferView dynamicBuffer(MemView<Type> view)
    {
      return S().dynamicBuffer(reinterpret_memView<uint8_t>(view), sizeof(Type));
    }

    Renderpass createRenderpass()
    {
      return S().createRenderpass();
    }

    ComputePipeline createComputePipeline(ComputePipelineDescriptor desc)
    {
      return S().createComputePipeline(desc);
    }

    GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc)
    {
      return S().createGraphicsPipeline(desc);
    }

    CommandGraph createGraph()
    {
      return CommandGraph();
    }

    Swapchain createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor = SwapchainDescriptor())
    {
      auto sc = S().createSwapchain(surface, descriptor);
      sc.setParent(this);
      return sc;
    }

    void adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor = SwapchainDescriptor())
    {
      S().adjustSwapchain(swapchain, descriptor);
    }

    TextureRTV acquirePresentableImage(Swapchain& swapchain)
    {
      return S().acquirePresentableImage(swapchain);
    }

    void submit(Swapchain& swapchain, CommandGraph graph)
    {
      S().submit(swapchain, graph);
    }

    void present(Swapchain& swapchain)
    {
      S().present(swapchain);
    }

    void garbageCollection()
    {
      S().garbageCollection();
    }
  };
};