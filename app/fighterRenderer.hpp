#pragma once

#include "core/src/system/time.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/global_debug.hpp"
#include "shaders/rectRenderer.if.hpp"
#include "renderer/blitter.hpp"
#include "renderer/imgui_renderer.hpp"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "core/src/external/imgiu/imgui.h"
#include "core/src/input/gamepad.hpp"
#include "faze/src/new_gfx/definitions.hpp"
#include "faze/src/helpers/pingpongTexture.hpp"
#include "core/src/system/AtomicBuffered.hpp"

#include "renderData.hpp"

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

    AtomicDoubleBuffered<vector<ColoredRect>> boxes;
  public:
    FighterRenderer(GraphicsSurface& surface, GpuDevice& gpu)
      : gpu(gpu)
      , swapdesc(SwapchainDescriptor()
        .formatType(FormatType::Unorm8RGBA)
        .colorspace(Colorspace::BT709)
        .bufferCount(3)
        .frameLatency(2)
        .presentMode(PresentMode::FifoRelaxed))
      , swap(gpu.createSwapchain(surface, swapdesc))
      , blitter(gpu)
      , fighterRenderpass(gpu.createRenderpass())
      , fighterPipe(gpu.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .setVertexShader("rectRenderer")
        .setPixelShader("rectRenderer")
        .setPrimitiveTopology(PrimitiveTopology::Triangle)
        .setDepthStencil(DepthStencilDescriptor()
          .setDepthEnable(false))))
      , imgRenderer(gpu)
    {
      timer.firstTick();
      m_resize = 0;

      boxes.writeValue(vector<ColoredRect>());
    }

    void resize()
    {
      m_resize++;
    }

    void updateBoxes(vector<ColoredRect> rects)
    {
      boxes.writeValue(rects);
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
      auto tex = gpu.acquirePresentableImage(swap);
      {
        auto& node = graph.createPass2("background");

        auto some = std::sin(timer.getFTime())*0.05f + 0.2f;

        node.clearRT(tex, sub(float4(1.f), float4(some, some + 0.04f, some + 0.09f, 0.f)));
      }
      {
        auto& node = graph.createPass2("boxes");

        auto vec = boxes.readValue();
        node.renderpass(fighterRenderpass);
        node.subpass(tex);

        constexpr const float thickness = 0.01f;
        auto createQuad = [](vector<float4>& vertices, float2 leftTop, float2 rightBottom)
        {
          vertices.push_back(float4(rightBottom, 0.f, 1.f));
          vertices.push_back(float4(leftTop.x, rightBottom.y, 0.f, 1.f));
          vertices.push_back(float4(leftTop, 0.f, 1.f));
          vertices.push_back(float4(rightBottom, 0.f, 1.f));
          vertices.push_back(float4(leftTop, 0.f, 1.f));
          vertices.push_back(float4(rightBottom.x, leftTop.y, 0.f, 1.f));
        };

        for (auto rec : vec)
        {
          vector<float4> vertices;

          rec.topleft = sub(rec.topleft, float2(0.0, 0.8));
          rec.rightBottom = sub(rec.rightBottom, float2(0.0, 0.8));

          {
            // leftside
            auto newLT = float2(rec.topleft.x - thickness, rec.topleft.y + thickness);
            auto newRB = float2(rec.topleft.x, rec.rightBottom.y);

            createQuad(vertices, newLT, newRB);
          }

          {
            // rightside
            auto newLT = float2(rec.rightBottom.x, rec.topleft.y);
            auto newRB = float2(rec.rightBottom.x + thickness, rec.rightBottom.y - thickness);

            createQuad(vertices, newLT, newRB);
          }

          {
            // top
            auto newLT = float2(rec.topleft.x, rec.topleft.y + thickness);
            auto newRB = float2(rec.rightBottom.x + thickness, rec.topleft.y);

            createQuad(vertices, newLT, newRB);
          }
          {
            // bottom
            auto newLT = float2(rec.topleft.x - thickness, rec.rightBottom.y);
            auto newRB = float2(rec.rightBottom.x, rec.rightBottom.y - thickness);

            createQuad(vertices, newLT, newRB);
          }

          auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Float32RGBA);

          auto binding = node.bind<::shader::RectShader>(fighterPipe);
          binding.constants.color = float4{ rec.color, 1.f };
          binding.srv(::shader::RectShader::vertices, verts);
          node.draw(binding, static_cast<unsigned>(vertices.size()), 1);
        }
        node.endRenderpass();
      }
      {
        auto& node = graph.createPass2("background");
        node.prepareForPresent(tex);
      }

      gpu.submit(swap, graph);
      gpu.present(swap);
      timer.tick();
    }
  };
}