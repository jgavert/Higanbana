#include "rendering.hpp"

#include <imgui.h>

using namespace higanbana;

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
  Renderer::Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev)
   : graphics(graphics)
   , dev(dev)
   , imgui(dev)
  {
    scdesc = SwapchainDescriptor()
      .formatType(FormatType::Unorm8RGBA)
      .colorspace(Colorspace::BT709)
      .bufferCount(3).presentMode(PresentMode::Fifo);

    auto bufferdesc = ResourceDescriptor()
      .setName("testBuffer1")
      .setFormat(FormatType::Raw32)
      .setCount(1024*1024)
      .setDimension(FormatDimension::Buffer)
      .setUsage(ResourceUsage::GpuReadOnly);

    auto bufferdesc2 = ResourceDescriptor()
      .setName("testBuffer2")
      .setFormat(FormatType::Float32)
      .setCount(1024*1024)
      .setDimension(FormatDimension::Buffer)
      .setUsage(ResourceUsage::GpuReadOnly);

    auto bufferdesc3 = ResourceDescriptor()
      .setName("testOutBuffer")
      .setFormat(FormatType::Float32RGBA)
      .setCount(1024*256)
      .setDimension(FormatDimension::Buffer)
      .setUsage(ResourceUsage::GpuRW);

    buffer = dev.createBuffer(bufferdesc);
    testSRV = dev.createBufferSRV(buffer);
    buffer2 = dev.createBuffer(bufferdesc2);
    test2SRV = dev.createBufferSRV(buffer2);
    buffer3 = dev.createBuffer(bufferdesc3);
    testOut = dev.createBufferUAV(buffer3);

    higanbana::ShaderInputDescriptor babyInf = ShaderInputDescriptor()
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
      .setSize(uint2(1280, 720))
      .setFormat(FormatType::Unorm8RGBA)
      .setUsage(ResourceUsage::RenderTargetRW));

    higanbana::ShaderInputDescriptor babyInf2 = ShaderInputDescriptor()
      .constants<ComputeConstants>()
      .readWrite(ShaderResourceType::Texture2D, "float4", "output");

    genTexCompute = dev.createComputePipeline(ComputePipelineDescriptor()
    .setLayout(babyInf2)
    .setShader("simpleEffect")
    .setThreadGroups(uint3(8, 8, 1)));

    higanbana::ShaderInputDescriptor blitInf = ShaderInputDescriptor()
      .constants<PixelConstants>()
      .readOnly(ShaderResourceType::ByteAddressBuffer, "vertexInput")
      .readOnly(ShaderResourceType::Texture2D, "float4", "texInput");
    composite = dev.createGraphicsPipeline(basicDescriptor
      .setVertexShader("blit")
      .setPixelShader("blit")
      .setLayout(blitInf));
    compositeRP = dev.createRenderpass();

    targetRT.resize(dev, ResourceDescriptor()
      .setSize(uint2(1280, 720))
      .setFormat(FormatType::Unorm8RGBA)
      .setUsage(ResourceUsage::RenderTarget));

    size_t textureSize = 1280 * 720;

    sBuf = dev.createBuffer(ResourceDescriptor()
      .setCount(textureSize)
      .setFormat(FormatType::Raw32)
      .setUsage(ResourceUsage::GpuRW));
      //.allowCrossAdapter(1));

    sBufSRV = dev.createBufferSRV(sBuf);
     
     /*
    sTex = dev.createTexture(ResourceDescriptor()
      .setSize(uint2(1280, 720))
      .setFormat(FormatType::Unorm8RGBA)
      .setUsage(ResourceUsage::GpuReadOnly)
      .allowCrossAdapter(1));
      */

    time.startFrame();
  }

  void Renderer::initWindow(higanbana::Window& window, higanbana::GpuInfo info)
  {
    surface = graphics.createSurface(window, info);
    swapchain = dev.createSwapchain(surface, scdesc);
  }

  int2 Renderer::windowSize()
  {
    if (swapchain.buffers().empty())
      return int2(1,1);
    return swapchain.buffers().front().desc().desc.size3D().xy();
  }

  void Renderer::windowResized()
  {
    dev.adjustSwapchain(swapchain, scdesc);

    auto& desc = swapchain.buffers().front().desc();
    proxyTex.resize(dev, ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(desc.desc.format)
      .setUsage(ResourceUsage::RenderTargetRW)
      .setName("proxyTex"));

    targetRT.resize(dev, ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(desc.desc.format)
      .setUsage(ResourceUsage::RenderTarget)
      .setName("targetRT"));
  }

  void Renderer::render()
  {
    // If you acquire, you must submit it. Next, try to first present empty image.
    // On vulkan, need to at least clear the image or we will just get error about it. (... well at least when the contents are invalid in the beginning.)
    std::optional<TextureRTV> obackbuffer = dev.acquirePresentableImage(swapchain);
    if (!obackbuffer.has_value())
    {
      HIGAN_LOGi( "No backbuffer available");
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
      consts.resx = proxyTex.desc().desc.width; 
      consts.resy = proxyTex.desc().desc.height;
      binding.constants(consts);

      node.dispatchThreads(binding, proxyTex.desc().desc.size3D());


      tasks.addPass(std::move(node));
    }
    /*
    {
      auto node = tasks.createPass("copyBuffer", QueueType::Dma);
      node.copy(sBuf, buffer);
      tasks.addPass(std::move(node));
    }

    {
      auto node = tasks.createPass("copyBuffer", QueueType::Compute);
      node.copy(sBuf, buffer);
      tasks.addPass(std::move(node));
    }*/
    {
      auto node = tasks.createPass("composite");
      node.acquirePresentableImage(swapchain);
      backbuffer.setOp(LoadOp::DontCare);
      float redcolor = std::sin(time.getFTime())*.5f + .5f;

      backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 0.2f });
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
        binding.bind("texInput", proxyTex.srv());

        node.draw(binding, 3);
      }
      node.endRenderpass();
      tasks.addPass(std::move(node));
    }

    {
      auto node = tasks.createPass("Triangle");
      float redcolor = std::sin(time.getFTime())*.5f + .5f;

      //backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 1.f });
      backbuffer.setOp(LoadOp::Load);
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

    // IMGUI
    {
      auto node = tasks.createPass("IMGui");
      imgui.render(dev, node, backbuffer);
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