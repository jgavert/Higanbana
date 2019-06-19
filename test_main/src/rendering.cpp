#include "rendering.hpp"

using namespace faze;

SHADER_STRUCT(PixelConstants,
  float resx;
  float resy;
  float time;
  int unused;
);

namespace app
{
  Renderer::Renderer(faze::GraphicsSubsystem& graphics, faze::GpuGroup& dev)
   : graphics(graphics)
   , dev(dev)
  {
    scdesc = SwapchainDescriptor()
      .formatType(FormatType::Unorm8RGBA)
      .colorspace(Colorspace::BT709)
      .bufferCount(3).presentMode(PresentMode::Fifo);

    auto bufferdesc = ResourceDescriptor()
      .setName("testBuffer1")
      .setFormat(FormatType::Raw32)
      .setWidth(32)
      .setDimension(FormatDimension::Buffer)
      .setUsage(ResourceUsage::GpuReadOnly);

    auto bufferdesc2 = ResourceDescriptor()
      .setName("testBuffer2")
      .setFormat(FormatType::Float32)
      .setWidth(32)
      .setDimension(FormatDimension::Buffer)
      .setUsage(ResourceUsage::GpuReadOnly);

    auto bufferdesc3 = ResourceDescriptor()
      .setName("testOutBuffer")
      .setFormat(FormatType::Float32RGBA)
      .setWidth(8)
      .setDimension(FormatDimension::Buffer)
      .setUsage(ResourceUsage::GpuRW);

    buffer = dev.createBuffer(bufferdesc);
    testSRV = dev.createBufferSRV(buffer);
    buffer2 = dev.createBuffer(bufferdesc2);
    test2SRV = dev.createBufferSRV(buffer2);
    buffer3 = dev.createBuffer(bufferdesc3);
    testOut = dev.createBufferUAV(buffer3);

    babyInf = ShaderInputDescriptor().constants<PixelConstants>();
    triangle = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
      .setVertexShader("Triangle")
      .setPixelShader("Triangle")
      .setLayout(babyInf)
      .setPrimitiveTopology(PrimitiveTopology::Triangle)
      .setRTVFormat(0, FormatType::Unorm8BGRA)
      .setRenderTargetCount(1)
      .setDepthStencil(DepthStencilDescriptor()
        .setDepthEnable(false)));

    triangleRP = dev.createRenderpass();
    time.startFrame();
  }

  void Renderer::initWindow(faze::Window& window, faze::GpuInfo info)
  {
    surface = graphics.createSurface(window, info);
    swapchain = dev.createSwapchain(surface, scdesc);
  }

  void Renderer::windowResized()
  {
    dev.adjustSwapchain(swapchain, scdesc);
  }

  void Renderer::render()
  {

    // If you acquire, you must submit it. Next, try to first present empty image.
    // On vulkan, need to at least clear the image or we will just get error about it. (... well at least when the contents are invalid in the beginning.)
    std::optional<TextureRTV> obackbuffer = dev.acquirePresentableImage(swapchain);
    if (!obackbuffer.has_value())
    {
      F_ILOG("", "No backbuffer available");
      return;
    }
    TextureRTV backbuffer = obackbuffer.value();
    CommandGraph tasks = dev.createGraph();

    {
      auto node = tasks.createPass("Triangle");
      node.acquirePresentableImage(swapchain);
      float redcolor = 0.5f;//std::sin(time.getFTime())*.5f + .5f;

      backbuffer.clearOp(float4{ 0.f, 0.f, redcolor, 1.f });
      node.renderpass(triangleRP, backbuffer);
      {
        auto binding = node.bind(triangle);

        PixelConstants consts{};
        consts.time = time.getFTime();
        consts.resx = backbuffer.desc().desc.width;
        consts.resy = backbuffer.desc().desc.height;
        binding.constants(consts);

        node.draw(binding, 3);
      }
      node.endRenderpass();

      tasks.addPass(std::move(node));
    }
    {
      auto node = tasks.createPass("preparePresent");

      node.prepareForPresent(backbuffer);
      tasks.addPass(std::move(node));
    }

    dev.submit(swapchain, tasks);
    dev.present(swapchain);
    time.tick();
  }
}