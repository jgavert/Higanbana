#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/platform/Window.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/time.hpp>
#include <higanbana/graphics/helpers/pingpongTexture.hpp>
#include "renderer/imgui.hpp"

namespace app
{
  class Renderer
  {
    higanbana::GraphicsSubsystem& graphics;
    higanbana::GpuGroup& dev;
    higanbana::GraphicsSurface surface;

    higanbana::SwapchainDescriptor scdesc;
    higanbana::Swapchain swapchain;
    // resources
    higanbana::Buffer buffer;
    higanbana::Buffer buffer2;
    higanbana::Buffer buffer3;
    higanbana::BufferSRV testSRV;
    higanbana::BufferSRV test2SRV;
    higanbana::BufferUAV testOut;

    higanbana::GraphicsPipeline triangle;

    higanbana::Renderpass triangleRP;

    higanbana::GraphicsPipeline opaque;
    higanbana::Renderpass opaqueRP;

    higanbana::PingPongTexture proxyTex;
    higanbana::ComputePipeline genTexCompute;

    higanbana::GraphicsPipeline composite;
    higanbana::Renderpass compositeRP;
    // info
    higanbana::WTime time;

    // shared textures
    higanbana::PingPongTexture targetRT;
    higanbana::Buffer sBuf;
    higanbana::BufferSRV sBufSRV;
    
    higanbana::Texture sTex;

    renderer::IMGui imgui;

    higanbana::CpuImage image;
    higanbana::Texture fontatlas;

    // gbuffer??
    higanbana::Texture depth;
    higanbana::TextureDSV depthDSV;

    // camera

    float3 position;
    float3 dir;
    float3 updir;
    float3 sideVec;
    quaternion direction;

  public:
    Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev);
    void initWindow(higanbana::Window& window, higanbana::GpuInfo info);
    int2 windowSize();
    void windowResized();
    void render();
    std::optional<higanbana::SubmitTiming> timings();
  };
}