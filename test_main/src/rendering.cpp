#include "rendering.hpp"
#include <core/math/utils.hpp>

using namespace faze;

SHADER_STRUCT(PixelConstants,
  float resx;
  float resy;
  float time;
  int unused;
);

SHADER_STRUCT(ComputeConstants,
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

    faze::ShaderInputDescriptor babyInf = ShaderInputDescriptor()
      .constants<PixelConstants>()
      .readOnly(ShaderResourceType::ByteAddressBuffer, "vertexInput");

    auto basicDescriptor = GraphicsPipelineDescriptor()
      .setVertexShader("Triangle")
      .setPixelShader("Triangle")
      .setLayout(babyInf)
      .setPrimitiveTopology(PrimitiveTopology::Triangle)
      .setRTVFormat(0, FormatType::Unorm8BGRA)
      .setRenderTargetCount(1)
      .setDepthStencil(DepthStencilDescriptor()
        .setDepthEnable(false));
    
    triangle = dev.createGraphicsPipeline(basicDescriptor);

    triangleRP = dev.createRenderpass();

    proxyTex.resize(dev, ResourceDescriptor()
      .setWidth(1280)
      .setHeight(720)
      .setFormat(FormatType::Unorm8RGBA)
      .setUsage(ResourceUsage::RenderTargetRW));

    faze::ShaderInputDescriptor babyInf2 = ShaderInputDescriptor()
      .constants<ComputeConstants>()
      .readWrite(ShaderResourceType::Texture2D, "float4", "output");

    genTexCompute = dev.createComputePipeline(ComputePipelineDescriptor()
    .setLayout(babyInf2)
    .setShader("simpleEffect")
    .setThreadGroups(uint3(8, 8, 1)));

    faze::ShaderInputDescriptor blitInf = ShaderInputDescriptor()
      .constants<PixelConstants>()
      .readOnly(ShaderResourceType::ByteAddressBuffer, "vertexInput")
      .readOnly(ShaderResourceType::Texture2D, "float4", "input");
    composite = dev.createGraphicsPipeline(basicDescriptor
      .setVertexShader("blit")
      .setPixelShader("blit")
      .setLayout(blitInf));
    compositeRP = dev.createRenderpass();

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

    auto& desc = swapchain.buffers().front().desc();
    proxyTex.resize(dev, ResourceDescriptor()
      .setWidth(desc.desc.width)
      .setHeight(desc.desc.height)
      .setFormat(desc.desc.format)
      .setUsage(ResourceUsage::RenderTargetRW));
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
      auto node = tasks.createPass("generate Texture");
      auto binding = node.bind(genTexCompute);
      binding.bind("output", proxyTex.uav());
      ComputeConstants consts{};
      consts.time = time.getFTime();
      consts.resx = backbuffer.desc().desc.width; 
      consts.resy = backbuffer.desc().desc.height;
      binding.constants(consts);

      unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(backbuffer.desc().desc.width), 8));
      unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(backbuffer.desc().desc.height), 8));

      node.dispatch(binding, uint3(x, y, 1));

      tasks.addPass(std::move(node));
    }

    {
      auto node = tasks.createPass("composite");
      node.acquirePresentableImage(swapchain);
      backbuffer.setOp(LoadOp::DontCare);
      node.renderpass(compositeRP, backbuffer);
      {
        auto binding = node.bind(composite);

        PixelConstants consts{};
        consts.time = time.getFTime();
        consts.resx = backbuffer.desc().desc.width; 
        consts.resy = backbuffer.desc().desc.height;
        binding.constants(consts);

        vector<float> vertexData = {
          -1.f, -1.f, 
          -1.0f, 3.f,
          3.0f, -1.0f};
        auto vert = dev.dynamicBuffer<float>(vertexData, FormatType::Raw32);

        binding.bind("vertexInput", vert);
        binding.bind("input", proxyTex.srv());

        node.draw(binding, 3);
      }
      node.endRenderpass();
      tasks.addPass(std::move(node));
    }

    {
      auto node = tasks.createPass("Triangle");
      float redcolor = std::sin(time.getFTime())*.5f + .5f;

      backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 1.f });
      node.renderpass(triangleRP, backbuffer);
      {
        auto binding = node.bind(triangle);

        vector<float> vertexData = {
          -0.8f, -0.8f, 
          0.0f, 0.8f,
          0.8f, -0.8f};
        auto vert = dev.dynamicBuffer<float>(vertexData, FormatType::Raw32);

        PixelConstants consts{};
        consts.time = time.getFTime();
        consts.resx = backbuffer.desc().desc.width; 
        consts.resy = backbuffer.desc().desc.height;
        binding.constants(consts);
        binding.bind("vertexInput", vert);

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