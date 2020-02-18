#include "rendering.hpp"

#include <higanbana/core/profiling/profiling.hpp>

#include <imgui.h>
#include <execution>
#include <algorithm>

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

SHADER_STRUCT(OpaqueConsts,
  float resx;
  float resy;
  float time;
  int stretchBoxes;
  float4x4 worldMat;
  float4x4 viewMat;
);

namespace app
{
int Renderer::loadMesh(MeshData& data) {
  return meshes.allocate(dev, worldRend.meshArgLayout(), worldMeshRend.meshArgLayout(), data);
}

void Renderer::unloadMesh(int index) {
  meshes.free(index);
}

int Renderer::loadTexture(TextureData& data) {
  return textures.allocate(dev, data.image);
}
void Renderer::unloadTexture(int index) {
  textures.free(index);
}

int Renderer::loadMaterial(MaterialData& material) {
  //materials.allocate()
  return 0;
}
void Renderer::unloadMaterial(int index) {
  //return materials.free(index);
}

Renderer::Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev)
  : graphics(graphics)
  , dev(dev)
  , imgui(dev)
  , worldRend(dev)
  , worldMeshRend(dev)
  , tsaa(dev)
  , blitter(dev)
  , cubes(dev) {
  scdesc = SwapchainDescriptor()
    .formatType(FormatType::Unorm8RGBA)
    .colorspace(Colorspace::BT709)
    .bufferCount(3).presentMode(PresentMode::Mailbox);

  auto basicDescriptor = GraphicsPipelineDescriptor()
    .setVertexShader("Triangle")
    .setPixelShader("Triangle")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setRTVFormat(0, FormatType::Unorm8BGRA)
    .setRenderTargetCount(1)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false));

  proxyTex.resize(dev, ResourceDescriptor()
    .setSize(uint2(1280, 720))
    .setFormat(FormatType::Unorm8RGBA)
    .setUsage(ResourceUsage::RenderTargetRW));

  higanbana::ShaderArgumentsLayoutDescriptor layoutdesc = ShaderArgumentsLayoutDescriptor()
    .readWrite(ShaderResourceType::Texture2D, "float4", "output");
  compLayout = dev.createShaderArgumentsLayout(layoutdesc);
  higanbana::PipelineInterfaceDescriptor babyInf2 = PipelineInterfaceDescriptor()
    .constants<ComputeConstants>()
    .shaderArguments(0, compLayout);

  genTexCompute = dev.createComputePipeline(ComputePipelineDescriptor()
  .setInterface(babyInf2)
  .setShader("simpleEffectAssyt")
  .setThreadGroups(uint3(8, 4, 1)));

  higanbana::ShaderArgumentsLayoutDescriptor layoutdesc2 = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::ByteAddressBuffer, "vertexInput")
    .readOnly(ShaderResourceType::Texture2D, "float4", "texInput");
  blitLayout = dev.createShaderArgumentsLayout(layoutdesc2);
  composite = dev.createGraphicsPipeline(basicDescriptor
    .setVertexShader("blit")
    .setPixelShader("blit")
    .setInterface(PipelineInterfaceDescriptor()
      .constants<PixelConstants>()
      .shaderArguments(0, blitLayout)));
  compositeRP = dev.createRenderpass();

  tsaaResolved.resize(dev, ResourceDescriptor()
    .setSize(uint2(1280, 720))
    .setFormat(FormatType::Unorm8RGBA)
    .setUsage(ResourceUsage::RenderTarget)
    .setName("tsaaResolved"));

  size_t textureSize = 1280 * 720;

  cameras = dev.createBuffer(ResourceDescriptor()
  .setCount(10)
  .setStructured<CameraSettings>()
  .setName("Cameras")
  .setUsage(ResourceUsage::GpuRW));
  cameraSRV = dev.createBufferSRV(cameras);
  cameraUAV = dev.createBufferUAV(cameras);


  staticDataArgs = dev.createShaderArguments(ShaderArgumentsDescriptor("camera and other static data", worldRend.staticDataLayout())
    .bind("cameras", cameraSRV));

  // GBuffer
  gbuffer = dev.createTexture(higanbana::ResourceDescriptor()
    .setSize(uint2(1280, 720))
    .setFormat(FormatType::Unorm16RGBA)
    .setUsage(ResourceUsage::RenderTargetRW)
    .setName("gbuffer"));
  gbufferRTV = dev.createTextureRTV(gbuffer);
  gbufferSRV = dev.createTextureSRV(gbuffer);
  depth = dev.createTexture(higanbana::ResourceDescriptor()
    .setSize(gbuffer.desc().desc.size3D())
    .setFormat(FormatType::Depth32)
    .setUsage(ResourceUsage::DepthStencil)
    .setName("opaqueDepth"));
  depthDSV = dev.createTextureDSV(depth);

  time.startFrame();
}

void Renderer::initWindow(higanbana::Window& window, higanbana::GpuInfo info) {
  surface = graphics.createSurface(window, info);
  swapchain = dev.createSwapchain(surface, scdesc);
}

int2 Renderer::windowSize() {
  if (swapchain.buffers().empty())
    return int2(1,1);
  return swapchain.buffers().front().desc().desc.size3D().xy();
}

void Renderer::windowResized() {
  dev.adjustSwapchain(swapchain, scdesc);

  auto& desc = swapchain.buffers().front().desc();
  proxyTex.resize(dev, ResourceDescriptor()
    .setSize(desc.desc.size3D())
    .setFormat(desc.desc.format)
    .setUsage(ResourceUsage::RenderTargetRW)
    .setName("proxyTex"));

  tsaaResolved.resize(dev, ResourceDescriptor()
    .setSize(desc.desc.size3D())
    .setFormat(desc.desc.format)
    .setUsage(ResourceUsage::GpuRW)
    .setName("tsaa current/history"));
  gbuffer = dev.createTexture(higanbana::ResourceDescriptor()
    .setSize(desc.desc.size3D())
    .setFormat(FormatType::Unorm16RGBA)
    .setUsage(ResourceUsage::RenderTargetRW)
    .setName("gbuffer"));
  gbufferRTV = dev.createTextureRTV(gbuffer);
  gbufferSRV = dev.createTextureSRV(gbuffer);
  depth = dev.createTexture(higanbana::ResourceDescriptor()
    .setSize(desc.desc.size3D())
    .setFormat(FormatType::Depth32)
    .setUsage(ResourceUsage::DepthStencil)
    .setName("opaqueDepth"));

  depthDSV = dev.createTextureDSV(depth);
}

std::optional<higanbana::SubmitTiming> Renderer::timings() {
  auto info = dev.submitTimingInfo();
  if (!info.empty())
    m_previousInfo = info.front();
  
  return m_previousInfo;
}

void Renderer::renderMeshes(higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances) {
  // upload data to gpu :P
  vector<CameraSettings> sets;
  sets.push_back(CameraSettings{viewMat});
  auto matUpdate = dev.dynamicBuffer<CameraSettings>(makeMemView(sets));
  node.copy(cameras, matUpdate);


  backbuffer.setOp(LoadOp::Load);
  depthDSV.clearOp({});
  worldRend.beginRenderpass(node, backbuffer, depthDSV);
  for (auto&& instance : instances)
  {
    auto& mesh = meshes[instance.meshId];
    worldRend.renderMesh(node, mesh.indices, staticDataArgs, mesh.args);
  }
  worldRend.endRenderpass(node);
}

void Renderer::renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances) {
  // upload data to gpu :P
  vector<CameraSettings> sets;
  sets.push_back(CameraSettings{viewMat});
  auto matUpdate = dev.dynamicBuffer<CameraSettings>(makeMemView(sets));
  node.copy(cameras, matUpdate);

  backbuffer.setOp(LoadOp::Load);
  depthDSV.clearOp({});
  
  worldMeshRend.beginRenderpass(node, backbuffer, depthDSV);
  for (auto&& instance : instances)
  {
    auto& mesh = meshes[instance.meshId];
    worldMeshRend.renderMesh(node, mesh.indices, staticDataArgs, mesh.meshArgs, mesh.meshlets.desc().desc.width);
  }
  node.endRenderpass();
}

void Renderer::render(RendererOptions options, ActiveCamera camera, higanbana::vector<InstanceDraw>& instances, int drawcalls, int drawsSplitInto, std::optional<higanbana::CpuImage>& heightmap) {
  if (swapchain.outOfDate()) // swapchain can end up being outOfDate
  {
    windowResized();
  }

  // If you acquire, you must submit it.
  std::optional<std::pair<int,TextureRTV>> obackbuffer = dev.acquirePresentableImage(swapchain);
  if (!obackbuffer.has_value())
  {
    HIGAN_LOGi( "No backbuffer available...\n");
    return;
  }
  TextureRTV backbuffer = obackbuffer.value().second;
  CommandGraph tasks = dev.createGraph();

  {
    auto node = tasks.createPass("generate Texture");

    auto args = dev.createShaderArguments(ShaderArgumentsDescriptor("Generate Texture Descriptors", compLayout)
      .bind("output", proxyTex.uav()));

    auto binding = node.bind(genTexCompute);
    ComputeConstants consts{};
    consts.time = time.getFTime();
    consts.resx = proxyTex.desc().desc.width; 
    consts.resy = proxyTex.desc().desc.height;
    binding.constants(consts);
    binding.arguments(0, args);

    node.dispatchThreads(binding, proxyTex.desc().desc.size3D());

    tasks.addPass(std::move(node));
  }

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

      auto args = dev.createShaderArguments(ShaderArgumentsDescriptor("blit descriptors", blitLayout)
        .bind("vertexInput", vert)
        .bind("texInput", proxyTex.srv()));

      binding.arguments(0, args);

      node.draw(binding, 3);
    }
    node.endRenderpass();
    tasks.addPass(std::move(node));
  }

  {
    //auto node = tasks.createPass("opaquePass");

    // camera forming...
    auto aspect = float(backbuffer.desc().desc.height)/float(backbuffer.desc().desc.width);
    float4x4 pers = math::perspectivelh(camera.fov, aspect, camera.minZ, camera.maxZ);
    float4x4 rot = math::rotationMatrixLH(camera.direction);
    float4x4 pos = math::translation(camera.position);
    auto perspective = math::mul(pos, math::mul(rot, pers));
    HIGAN_CPU_BRACKET("user - outerloop");
    vector<float> vertexData = {
      1.0f, -1.f, -1.f,
      1.0f, -1.f, 1.f, 
      1.0f, 1.f, -1.f,
      1.0f, 1.f, 1.f,
      -1.0f, -1.f, -1.f,
      -1.0f, -1.f, 1.f, 
      -1.0f, 1.f, -1.f,
      -1.0f, 1.f, 1.f,
    };

    auto vert = dev.dynamicBuffer<float>(vertexData, FormatType::Raw32);
    vector<uint16_t> indexData = {
      1, 0, 2,
      1, 2, 3,
      5, 4, 0,
      5, 0, 1,
      7, 2, 6,
      7, 3, 2,
      5, 6, 4,
      5, 7, 6,
      6, 0, 4,
      6, 2, 0,
      7, 5, 1,
      7, 1, 3,
    };
    auto ind = dev.dynamicBuffer<uint16_t>(indexData, FormatType::Uint16);

    auto args = dev.createShaderArguments(ShaderArgumentsDescriptor("Opaque Arguments", cubes.getLayout())
      .bind("vertexInput", vert));
    if (heightmap && instances.empty())
    {
      int pixelsToDraw = drawcalls;
      backbuffer.setOp(LoadOp::Load);
      depthDSV.clearOp({});
      //auto node = tasks.createPass("opaquePass - cubes");
      
      vector<std::tuple<CommandGraphNode, int, int>> nodes;
      int stepSize = std::max(1, int((float(pixelsToDraw+1) / float(drawsSplitInto))+0.5f));
      for (int i = 0; i < pixelsToDraw; i+=stepSize)
      {
        if (i+stepSize > pixelsToDraw)
        {
          stepSize = stepSize - (i+stepSize - pixelsToDraw);
        }
        nodes.push_back(std::make_tuple(tasks.createPass("opaquePass - cubes"), i, stepSize));
      }


      std::for_each(std::execution::par_unseq, std::begin(nodes), std::end(nodes), [&](std::tuple<CommandGraphNode, int, int>& node)
      {
        HIGAN_CPU_BRACKET("user - innerloop");
        auto depth = depthDSV;
        if (std::get<1>(node) == 0)
          depth.setOp(LoadOp::Clear);
        else
          depth.setOp(LoadOp::Load);
        
        cubes.drawHeightMapInVeryStupidWay2(dev, time.getFTime(), std::get<0>(node), camera.position, perspective, backbuffer, depth, heightmap.value(), ind, args, pixelsToDraw, std::get<1>(node), std::get<1>(node)+std::get<2>(node));
      });

      for (auto& node : nodes)
      {
        tasks.addPass(std::move(std::get<0>(node)));
      }
    }
    else if (instances.empty())
    {
      backbuffer.setOp(LoadOp::Load);
      depthDSV.clearOp({});
      
      vector<std::tuple<CommandGraphNode, int, int>> nodes;
      int stepSize = std::max(1, int((float(drawcalls+1) / float(drawsSplitInto))+0.5f));
      for (int i = 0; i < drawcalls; i+=stepSize)
      {
        if (i+stepSize > drawcalls)
        {
          stepSize = stepSize - (i+stepSize - drawcalls);
        }
        nodes.push_back(std::make_tuple(tasks.createPass("opaquePass - cubes"), i, stepSize));
      }
      std::for_each(std::execution::par_unseq, std::begin(nodes), std::end(nodes), [&](std::tuple<CommandGraphNode, int, int>& node)
      {
        HIGAN_CPU_BRACKET("user - innerloop");
        auto depth = depthDSV;
        if (std::get<1>(node) == 0)
          depth.setOp(LoadOp::Clear);
        else
          depth.setOp(LoadOp::Load);
        
        cubes.oldOpaquePass2(dev, time.getFTime(), std::get<0>(node), perspective, backbuffer, depth,ind, args,  drawcalls, std::get<1>(node),  std::get<1>(node)+std::get<2>(node));
      });

      for (auto& node : nodes)
      {
        tasks.addPass(std::move(std::get<0>(node)));
      }
    }
    else
    {
      auto node = tasks.createPass("opaquePass - ecs");
      if (options.allowMeshShaders)
        renderMeshesWithMeshShaders(node, perspective, backbuffer, instances);
      else
        renderMeshes(node, perspective, backbuffer, instances);
      tasks.addPass(std::move(node));
    }
    
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
  {
    HIGAN_CPU_BRACKET("Present");
    dev.present(swapchain, obackbuffer.value().first);
  }
  time.tick();
}
}