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

namespace faze
{
  class AppRenderer
  {
    // statistics
    WTime timer;

    // core
    GpuDevice& gpu;

    // static resources
    PingPongTexture raymarchTexture;
    Texture postEffectTex;
    TextureSRV postEffectSrv;
    TextureUAV postEffectUav;
    Texture postEffectLDSTex;
    TextureSRV postEffectLDSSrv;
    TextureUAV postEffectLDSUav;

    // renderers

    renderer::TexturePass<::shader::PostEffect> postPass;
    renderer::TexturePass<::shader::PostEffect> postPassLDS;
    renderer::Blitter blitter;
    renderer::ImGui imgRenderer;
    ComputePipeline raymarcher;

    Renderpass triangleRenderpass;
    GraphicsPipeline trianglePipe;
  public:
    AppRenderer(GraphicsSurface& surface, GpuDevice& gpu, int2 resolution)
      : gpu(gpu)
      , postPass(gpu, "posteffect", uint3(64, 1, 1))
      , postPassLDS(gpu, "posteffectLDS", uint3(64, 1, 1))
      , blitter(gpu)
      , imgRenderer(gpu)
      , raymarcher(gpu.createComputePipeline(ComputePipelineDescriptor()
        .setShader("raymarcher")
        .setThreadGroups(uint3(8, 8, 1))))
      , triangleRenderpass(gpu.createRenderpass())
      , trianglePipe(gpu.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .setVertexShader("triangle")
        .setPixelShader("triangle")
        .setPrimitiveTopology(PrimitiveTopology::Triangle)
        .setDepthStencil(DepthStencilDescriptor()
          .setDepthEnable(false))))
    {
      resize(resolution);
    }
    void resize(int2 resolution)
    {
      auto raymarchedDesc = ResourceDescriptor()
        .setName("raymarchedDesc")
        .setFormat(FormatType::Unorm16RGBA)
        .setSize(resolution)
        .setUsage(ResourceUsage::GpuRW);
      raymarchTexture = PingPongTexture(gpu, raymarchedDesc);
      postEffectTex = gpu.createTexture(raymarchedDesc.setName("postEffect"));
      postEffectSrv = gpu.createTextureSRV(postEffectTex);
      postEffectUav = gpu.createTextureUAV(postEffectTex);

      postEffectLDSTex = gpu.createTexture(raymarchedDesc.setName("postEffectLDS"));
      postEffectLDSSrv = gpu.createTextureSRV(postEffectLDSTex);
      postEffectLDSUav = gpu.createTextureUAV(postEffectLDSTex);
    }

    void render()
    {
      /*
      CommandGraph tasks = gpu.createGraph();

      raymarchTexture.next();
      uint2 iRes = raymarchTexture.desc().size().xy();
      {
        auto type = CommandGraphNode::NodeType::Graphics;

        auto node = tasks.createPass("compute!", type);

        auto binding = node.bind<::shader::TextureTest>(raymarcher);
        binding.constants = {};
        binding.constants.iResolution = iRes;
        binding.constants.iFrame = static_cast<int>(frame);
        binding.constants.iTime = time.getFTime();
        binding.constants.iPos = position;
        binding.constants.iDir = dir;
        binding.constants.iUpDir = updir;
        binding.constants.iSideDir = sideVec;
        binding.uav(::shader::TextureTest::output, raymarched.uav());

        unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(iRes.x), 8));
        unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(iRes.y), 8));
        node.dispatch(binding, uint3{ x, y, 1 });

        if (enableAsyncCompute)
        {
          node.queryCounters([&](MemView<std::pair<std::string, double>> view)
          {
            asyncCompute.clear();
            for (auto&& it : view)
            {
              asyncCompute.push_back(std::make_pair(it.first, it.second));
            }
          });
        }

        tasks.addPass(std::move(node));
      }
      //if (inputs.isPressedThisFrame('3', 2))

      {
        auto node = tasks.createPass("clear");
        node.acquirePresentableImage(swapchain);
        node.clearRT(backbuffer, float4{ std::sin(time.getFTime())*.5f + .5f, 0.f, 0.f, 0.f });
        //node.clearRT(texRtv, vec4{ std::sin(float(frame)*0.01f)*.5f + .5f, std::sin(float(frame)*0.01f)*.5f + .5f, 0.f, 1.f });
        tasks.addPass(std::move(node));
      }

      iRes.y = 1;
      postPass.compute(tasks, texUav2, raymarched.srv(), iRes);
      iRes.x *= 64;
      postPass2.compute(tasks, texUav3, raymarched.srv(), iRes);

      {
        // we have pulsing red color background, draw a triangle on top of it !
        auto node = tasks.createPass("Triangle!");
        node.renderpass(triangleRenderpass);
        backbuffer.setOp(LoadOp::DontCare);
        node.subpass(backbuffer);
        backbuffer.setOp(LoadOp::DontCare);

        vector<float4> vertices;
        vertices.push_back(float4{ -1.f, -1.f, 1.f, 1.f });
        vertices.push_back(float4{ -1.0f, 3.f, 1.f, 1.f });
        vertices.push_back(float4{ 3.f, -1.f, 1.f, 1.f });

        auto verts = dev.dynamicBuffer(makeMemView(vertices), FormatType::Float32RGBA);

        auto binding = node.bind<::shader::Triangle>(trianglePipe);
        //binding.constants.color = float4{ 0.f, 0.f, std::sin(float(frame)*0.01f + 1.0f)*.5f + .5f, 1.f };
        binding.constants.color = float4{ 0.f, 0.f, 0.f, 1.f };
        binding.constants.colorspace = static_cast<int>(swapchain.impl()->displayCurve());
        binding.srv(::shader::Triangle::vertices, verts);
        binding.srv(::shader::Triangle::yellow, raymarched.srv());
        node.draw(binding, 3, 1);
        node.endRenderpass();
        tasks.addPass(std::move(node));
      }
      //float heightMulti = 1.f; // float(testSrv.desc().desc.height) / float(testSrv.desc().desc.width);
      //blit.blitImage(dev, tasks, backbuffer, testSrv, renderer::Blitter::FitMode::Fit);

      auto bpos = div(uint2(backbuffer.desc().desc.width, backbuffer.desc().desc.height), 2u);
      blit.blit(dev, tasks, backbuffer, texSrv2, int2(bpos), int2(bpos));
      blit.blit(dev, tasks, backbuffer, texSrv3, int2(bpos.x, 0), int2(bpos));
      */
    }
  };
}