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
      buf.setParent(this);
      return buf;
    }

    Texture createTexture(ResourceDescriptor descriptor)
    {
      auto tex = S().createTexture(descriptor);
      tex.setParent(this);
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

    CommandGraph createGraph()
    {
      return CommandGraph();
    }

    Swapchain createSwapchain(GraphicsSurface& surface, PresentMode mode = PresentMode::Fifo, FormatType format = FormatType::Unorm8x4, int bufferCount = 2)
    {
      auto sc = S().createSwapchain(surface, mode, format, bufferCount);
      sc.setParent(this);
      return sc;
    }

    void adjustSwapchain(Swapchain& swapchain, PresentMode mode = PresentMode::Unknown, FormatType format = FormatType::Unknown, int bufferCount = -1)
    {
      S().adjustSwapchain(swapchain, mode, format, bufferCount);
    }

    TextureRTV acquirePresentableImage(Swapchain& swapchain)
    {
      return S().acquirePresentableImage(swapchain);
    }

    void submit(CommandGraph graph)
    {
      S().submit(graph);
    }

    void present(Swapchain& swapchain)
    {
      S().present(swapchain);
    }
  };
};