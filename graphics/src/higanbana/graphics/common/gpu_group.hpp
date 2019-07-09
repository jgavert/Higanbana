#pragma once
#include "higanbana/graphics/common/device_group_data.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/buffer.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/semaphore.hpp"
#include "higanbana/graphics/common/swapchain.hpp"
#include "higanbana/graphics/common/commandgraph.hpp"
#include "higanbana/graphics/common/commandlist.hpp"
#include "higanbana/graphics/common/cpuimage.hpp"
#include "higanbana/graphics/common/prototypes.hpp"

#include <optional>

namespace higanbana
{
  class GpuGroup : private backend::SharedState<backend::DeviceGroupData>
  {
  public:
    GpuGroup() = default;
    GpuGroup(vector<std::shared_ptr<backend::prototypes::DeviceImpl>> impl, vector<GpuInfo> infos)
    {
      makeState(impl, infos);
    }
    GpuGroup(StatePtr state)
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
      if (descriptor.desc.dimension == FormatDimension::Buffer
      || descriptor.desc.dimension == FormatDimension::Unknown)
      {
        descriptor = descriptor.setDimension(FormatDimension::Texture2D);
      }
      auto tex = S().createTexture(descriptor);
      return tex;
    }

    Texture createTexture(CpuImage& image)
    {
      auto tex = S().createTexture(image.desc());
      S().uploadInitialTexture(tex, image);
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

    template <typename Type>
    DynamicBufferView dynamicImage(MemView<Type> view, size_t rowPitch)
    {
      return S().dynamicImage(reinterpret_memView<uint8_t>(view), static_cast<unsigned>(rowPitch));
    }

    ReadbackBuffer readback()
    {
      return ReadbackBuffer();
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
      return S().startCommandGraph();
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

    std::optional<TextureRTV> acquirePresentableImage(Swapchain& swapchain)
    {
      return S().acquirePresentableImage(swapchain);
    }

    TextureRTV* tryAcquirePresentableImage(Swapchain& swapchain)
    {
        return S().tryAcquirePresentableImage(swapchain);
    }

    void submit(Swapchain& swapchain, CommandGraph graph)
    {
      S().submit(swapchain, graph);
    }

    void submit(CommandGraph graph)
    {
      S().submit(std::optional<Swapchain>(), graph);
    }

    void present(Swapchain& swapchain)
    {
      S().present(swapchain);
    }

    void garbageCollection()
    {
      S().garbageCollection();
    }

    void waitGpuIdle()
    {
      S().waitGpuIdle();
    }

    bool alive()
    {
      return valid();
    }
  };
};