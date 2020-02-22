#include "rendering.hpp"

#include <higanbana/core/profiling/profiling.hpp>

#include <imgui.h>
#include <execution>
#include <algorithm>

using namespace higanbana;

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
  , m_camerasLayout(dev.createShaderArgumentsLayout(higanbana::ShaderArgumentsLayoutDescriptor()
      .readOnly<CameraSettings>(ShaderResourceType::StructuredBuffer, "cameras")))
  , cubes(dev)
  , imgui(dev)
  , worldRend(dev, m_camerasLayout)
  , worldMeshRend(dev, m_camerasLayout)
  , tsaa(dev)
  , blitter(dev)
  , genImage(dev, "simpleEffectAssyt", uint3(8,8,1))
  , particleSimulation(dev, m_camerasLayout) {
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


  cameraArgs = dev.createShaderArguments(ShaderArgumentsDescriptor("camera and other static data", m_camerasLayout)
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

void Renderer::renderMeshes(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances) {
  backbuffer.setOp(LoadOp::Load);
  depthDSV.clearOp({});
  worldRend.beginRenderpass(node, backbuffer, depthDSV);
  for (auto&& instance : instances)
  {
    auto& mesh = meshes[instance.meshId];
    worldRend.renderMesh(node, mesh.indices, cameraArgs, mesh.args);
  }
  worldRend.endRenderpass(node);
}

void Renderer::renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances) {
  backbuffer.setOp(LoadOp::Load);
  depthDSV.clearOp({});
  
  worldMeshRend.beginRenderpass(node, backbuffer, depthDSV);
  for (auto&& instance : instances)
  {
    auto& mesh = meshes[instance.meshId];
    worldMeshRend.renderMesh(node, mesh.indices, cameraArgs, mesh.meshArgs, mesh.meshlets.desc().desc.width);
  }
  node.endRenderpass();
}

void Renderer::render(LBS& lbs, RendererOptions options, ActiveCamera camera, higanbana::vector<InstanceDraw>& instances, int drawcalls, int drawsSplitInto, std::optional<higanbana::CpuImage>& heightmap) {
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
    auto ndoe = tasks.createPass("simulate particles");
    particleSimulation.simulate(dev, ndoe);
    tasks.addPass(std::move(ndoe));
  }
  {
    auto ndoe = tasks.createPass("copy cameras");
    vector<CameraSettings> sets;
    auto aspect = float(backbuffer.desc().desc.height)/float(backbuffer.desc().desc.width);
    float4x4 pers = math::perspectivelh(camera.fov, aspect, camera.minZ, camera.maxZ);
    float4x4 rot = math::rotationMatrixLH(camera.direction);
    float4x4 pos = math::translation(camera.position);
    auto perspective = math::mul(pos, math::mul(rot, pers));
    sets.push_back(CameraSettings{perspective});
    auto matUpdate = dev.dynamicBuffer<CameraSettings>(makeMemView(sets));
    ndoe.copy(cameras, matUpdate);
    tasks.addPass(std::move(ndoe));
  }

  {
    auto node = tasks.createPass("generate Texture");
    genImage.generate(dev, node, proxyTex.uav());
    tasks.addPass(std::move(node));
  }

  {
    auto node = tasks.createPass("composite");
    node.acquirePresentableImage(swapchain);
    float redcolor = std::sin(time.getFTime())*.5f + .5f;

    backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 0.2f });
    blitter.beginRenderpass(node, backbuffer);
    blitter.blitImage(dev, node, proxyTex.srv(), renderer::Blitter::FitMode::Fill);
    node.endRenderpass();
    tasks.addPass(std::move(node));
  }

  {
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
      if (!options.unbalancedCubes)
      {
        int stepSize = std::max(1, int((float(drawcalls+1) / float(drawsSplitInto))+0.5f));
        for (int i = 0; i < drawcalls; i+=stepSize)
        {
          if (i+stepSize > drawcalls)
          {
            stepSize = stepSize - (i+stepSize - drawcalls);
          }
          nodes.push_back(std::make_tuple(tasks.createPass("opaquePass - cubes"), i, stepSize));
        }
      }
      else
      {
        int cubesLeft = drawcalls;
        int cubesDrawn = 0;
        vector<int> reverseDraw;
        for (int i = 0; i < drawsSplitInto-1; i++)
        {
          auto split = cubesLeft/2;
          cubesLeft -= split;
          reverseDraw.emplace_back(split);
        }
        reverseDraw.push_back(cubesLeft);
        for (int i = reverseDraw.size()-1; i >= 0; i--)
        {
          nodes.push_back(std::make_tuple(tasks.createPass("opaquePass - cubes"), cubesDrawn, reverseDraw[i]));
          cubesDrawn += reverseDraw[i];
        } 
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
        renderMeshesWithMeshShaders(node, backbuffer, instances);
      else
        renderMeshes(node, backbuffer, instances);
      tasks.addPass(std::move(node));
    }
  }

  // particles draw
  {
    auto node = tasks.createPass("particles - draw");
    particleSimulation.render(dev, node, backbuffer, depthDSV, cameraArgs);
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

  if (options.submitSingleThread)
    dev.submitExperimental(swapchain, tasks, ThreadedSubmission::Sequenced);
  else if (options.submitExperimental)
    dev.submitExperimental(swapchain, tasks, ThreadedSubmission::ParallelUnsequenced);
  else if (options.submitLBS)
    dev.submitLBS(lbs, swapchain, tasks, ThreadedSubmission::ParallelUnsequenced);
  else 
    dev.submit(swapchain, tasks);
  {
    HIGAN_CPU_BRACKET("Present");
    dev.present(swapchain, obackbuffer.value().first);
  }
  time.tick();
}
}