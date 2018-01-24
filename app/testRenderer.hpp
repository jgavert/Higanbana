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

    void render(TextureRTV target)
    {
    }
  };
}