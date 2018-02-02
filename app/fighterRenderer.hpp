#pragma once

#include "core/src/system/time.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/global_debug.hpp"

#include "shaders/textureTest.if.hpp"
#include "shaders/posteffect.if.hpp"
#include "shaders/triangle.if.hpp"
#include "shaders/buffertest.if.hpp"

#include "renderer/blitter.hpp"
#include "renderer/imgui_renderer.hpp"

#include "renderer/texture_pass.hpp"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "core/src/external/imgiu/imgui.h"

#include "faze/src/new_gfx/common/cpuimage.hpp"
#include "faze/src/new_gfx/common/image_loaders.hpp"

#include "core/src/input/gamepad.hpp"

#include "faze/src/new_gfx/definitions.hpp"

#include "faze/src/helpers/pingpongTexture.hpp"

#include "core/src/system/AtomicBuffered.hpp"

namespace faze
{
  class FighterRenderer
  {
    // statistics
    WTime timer;

    // core
    GpuDevice& gpu;

    // static resources
    SwapchainDescriptor swapdesc;
    Swapchain swap;

    // renderers
    renderer::Blitter blitter;
    Renderpass fighterRenderpass;
    GraphicsPipeline fighterPipe;
    // menus
    renderer::ImGui imgRenderer;

    //
    std::atomic<int> m_resize;
  public:
    FighterRenderer(GraphicsSurface& surface, GpuDevice& gpu)
      : gpu(gpu)
      , swapdesc(SwapchainDescriptor()
        .formatType(FormatType::Unorm8RGBA)
        .colorspace(Colorspace::BT709)
        .bufferCount(3)
        .frameLatency(2)
        .presentMode(PresentMode::Fifo))
      , swap(gpu.createSwapchain(surface, swapdesc))
      , blitter(gpu)
      , fighterRenderpass(gpu.createRenderpass())
      , fighterPipe(gpu.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .setVertexShader("triangle")
        .setPixelShader("triangle")
        .setPrimitiveTopology(PrimitiveTopology::Triangle)
        .setDepthStencil(DepthStencilDescriptor()
          .setDepthEnable(false))))
      , imgRenderer(gpu)
    {
      timer.firstTick();
      m_resize = 0;
    }

    void resize()
    {
      m_resize++;
    }

    void render()
    {
      timer.startFrame();
      while (m_resize > 0)
      {
        gpu.adjustSwapchain(swap, swapdesc);
        m_resize--;
      }

      auto graph = gpu.createGraph();

      {
        auto& node = graph.createPass2("change color");
        auto tex = gpu.acquirePresentableImage(swap);

        auto some = std::sin(float(timer.getFrame())*0.05f);

        node.clearRT(tex, float4(some, 0.f, 0.f, 1.f));
        node.prepareForPresent(tex);
      }
      gpu.submit(swap, graph);
      gpu.present(swap);
      timer.tick();
    }
  };
}