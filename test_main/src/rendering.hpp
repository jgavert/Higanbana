#pragma once
#include <graphics/GraphicsCore.hpp>
#include <core/filesystem/filesystem.hpp>
#include <core/platform/Window.hpp>
#include <core/datastructures/proxy.hpp>
#include <core/system/time.hpp>

namespace app
{
  class Renderer
  {
    faze::GraphicsSubsystem& graphics;
    faze::GpuGroup& dev;
    faze::GraphicsSurface surface;

    faze::SwapchainDescriptor scdesc;
    faze::Swapchain swapchain;
    // resources
    faze::Buffer buffer;
    faze::Buffer buffer2;
    faze::Buffer buffer3;
    faze::BufferSRV testSRV;
    faze::BufferSRV test2SRV;
    faze::BufferUAV testOut;

    faze::ShaderInputDescriptor babyInf;
    faze::GraphicsPipeline triangle;

    faze::Renderpass triangleRP;

    // info
    faze::WTime time;
  public:
    Renderer(faze::GraphicsSubsystem& graphics, faze::GpuGroup& dev);
    void initWindow(faze::Window& window, faze::GpuInfo info);
    void windowResized();
    void render();
  };
}