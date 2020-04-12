#include "rendering.hpp"

#include <higanbana/core/profiling/profiling.hpp>

#include <imgui.h>
#include <execution>
#include <algorithm>

using namespace higanbana;

namespace app
{
int Renderer::loadBuffer(BufferData& data) {
  return meshes.allocateBuffer(dev, data);
}

void Renderer::unloadBuffer(int index) {
  meshes.freeBuffer(index);
}
int Renderer::loadMesh(MeshData& data, int buffers[5]) {
  return meshes.allocate(dev, worldRend.meshArgLayout(), worldMeshRend.meshArgLayout(), data, buffers);
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
  return materials.allocate(dev, material);
}
void Renderer::unloadMaterial(int index) {
  materials.free(index);
}

Renderer::Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev)
  : graphics(graphics)
  , dev(dev)
  , m_camerasLayout(dev.createShaderArgumentsLayout(higanbana::ShaderArgumentsLayoutDescriptor()
      .readOnly<CameraSettings>(ShaderResourceType::StructuredBuffer, "cameras")))
  , textures(dev)
  , cubes(dev)
  , imgui(dev)
  , worldRend(dev, m_camerasLayout, textures.bindlessLayout())
  , worldMeshRend(dev, m_camerasLayout, textures.bindlessLayout())
  , tsaa(dev)
  , tonemapper(dev)
  , blitter(dev)
  , genImage(dev, "simpleEffectAssyt", uint3(8,8,1))
  , particleSimulation(dev, m_camerasLayout) {
  HIGAN_CPU_FUNCTION_SCOPE();
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

  size_t textureSize = 1280 * 720;

  cameras = dev.createBuffer(ResourceDescriptor()
  .setCount(8*4)
  .setStructured<CameraSettings>()
  .setName("Cameras")
  .setUsage(ResourceUsage::GpuRW));
  cameraSRV = dev.createBufferSRV(cameras);
  cameraUAV = dev.createBufferUAV(cameras);


  cameraArgs = dev.createShaderArguments(ShaderArgumentsDescriptor("camera and other static data", m_camerasLayout)
    .bind("cameras", cameraSRV));
  ResourceDescriptor desc = ResourceDescriptor()
    .setSize(uint3(1280, 720, 1))
    .setFormat(FormatType::Unorm8BGRA);
  resizeExternal(desc);
}

void Renderer::initWindow(higanbana::Window& window, higanbana::GpuInfo info) {
  HIGAN_CPU_FUNCTION_SCOPE();
  surface = graphics.createSurface(window, info);
  swapchain = dev.createSwapchain(surface, scdesc);
  resizeExternal(swapchain.buffers().front().texture().desc());
}

int2 Renderer::windowSize() {
  if (swapchain.buffers().empty())
    return int2(1,1);
  return swapchain.buffers().front().desc().desc.size3D().xy();
}

void Renderer::resizeExternal(higanbana::ResourceDescriptor& desc) {
  proxyTex.resize(dev, ResourceDescriptor()
    .setSize(desc.desc.size3D())
    .setFormat(FormatType::Float16RGBA)
    .setUsage(ResourceUsage::RenderTargetRW)
    .setName("proxyTex"));
}

std::optional<higanbana::SubmitTiming> Renderer::timings() {
  auto info = dev.submitTimingInfo();
  if (!info.empty())
    m_previousInfo = info.front();
  
  return m_previousInfo;
}

void Renderer::renderMeshes(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth, higanbana::ShaderArguments materials, int cameraIndex, int prevCamera, higanbana::vector<InstanceDraw>& instances) {
  HIGAN_CPU_FUNCTION_SCOPE();
  backbuffer.setOp(LoadOp::Load);
  depth.clearOp({});
  worldRend.beginRenderpass(node, backbuffer, motionVecs, depth);
  auto binding = worldRend.bindPipeline(node);
  binding.arguments(0, cameraArgs);
  binding.arguments(2, materials);
  for (auto&& instance : instances)
  {
    auto& mesh = meshes[instance.meshId];
    binding.arguments(1, mesh.args);
    worldRend.renderMesh(node, binding, mesh.indices, cameraIndex, prevCamera, instance.materialId, backbuffer.desc().desc.size3D().xy(), instance.mat);
  }
  worldRend.endRenderpass(node);
}

void Renderer::renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, higanbana::ShaderArguments materials, int cameraIndex, higanbana::vector<InstanceDraw>& instances) {
  HIGAN_CPU_FUNCTION_SCOPE();
  backbuffer.setOp(LoadOp::Load);
  depth.clearOp({});
  
  worldMeshRend.beginRenderpass(node, backbuffer, depth);
  for (auto&& instance : instances)
  {
    auto& mesh = meshes[instance.meshId];
    worldMeshRend.renderMesh(node, mesh.indices, cameraArgs, mesh.meshArgs, materials, mesh.meshlets.desc().desc.width, cameraIndex);
  }
  node.endRenderpass();
}

void Renderer::renderScene(higanbana::CommandNodeVector& tasks, higanbana::WTime& time, const RendererOptions& rendererOptions, const Renderer::SceneArguments& scene, higanbana::vector<InstanceDraw>& instances) {
  HIGAN_CPU_FUNCTION_SCOPE();
  {
    auto node = tasks.createPass("composite");
    float redcolor = std::sin(time.getFTime())*.5f + .5f;
    auto gbufferRTV = scene.gbufferRTV;
    gbufferRTV.clearOp(float4{ 0.f, redcolor, 0.f, 0.2f });
    blitter.beginRenderpass(node, gbufferRTV);
    blitter.blitImage(dev, node, gbufferRTV, proxyTex.srv(), renderer::Blitter::FitMode::Fill);
    node.endRenderpass();
    tasks.addPass(std::move(node));
  }

  {
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
    if (instances.empty())
    {
      HIGAN_CPU_BRACKET("draw cubes - outer loop");
      auto gbufferRTV = scene.gbufferRTV;
      auto depth = scene.depth;
      gbufferRTV.setOp(LoadOp::Load);
      depth.clearOp({});
      
      vector<std::tuple<CommandGraphNode, int, int>> nodes;
      if (!rendererOptions.unbalancedCubes)
      {
        int stepSize = std::max(1, int((float(scene.drawcalls+1) / float(scene.drawsSplitInto))+0.5f));
        for (int i = 0; i < scene.drawcalls; i+=stepSize)
        {
          if (i+stepSize > scene.drawcalls)
          {
            stepSize = stepSize - (i+stepSize - scene.drawcalls);
          }
          nodes.push_back(std::make_tuple(tasks.createPass("opaquePass - cubes"), i, stepSize));
        }
      }
      else
      {
        int cubesLeft = scene.drawcalls;
        int cubesDrawn = 0;
        vector<int> reverseDraw;
        for (int i = 0; i < scene.drawsSplitInto-1; i++)
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
        HIGAN_CPU_BRACKET("draw cubes - inner pass");
        auto ldepth = depth;
        if (std::get<1>(node) == 0)
          ldepth.setOp(LoadOp::Clear);
        else
          ldepth.setOp(LoadOp::Load);
        
        cubes.oldOpaquePass2(dev, time.getFTime(), std::get<0>(node), scene.perspective, gbufferRTV, ldepth, ind, args, scene.drawcalls, std::get<1>(node),  std::get<1>(node)+std::get<2>(node));
      });

      for (auto& node : nodes)
      {
        tasks.addPass(std::move(std::get<0>(node)));
      }
    }
    else
    {
      auto node = tasks.createPass("opaquePass - ecs");
      auto gbufferRTV = scene.gbufferRTV;
      auto depth = scene.depth;
      auto moti = scene.motionVectors;
      HIGAN_ASSERT(scene.materials, "wtf!");
      moti.setOp(LoadOp::Clear);
      if (rendererOptions.allowMeshShaders && scene.options.useMeshShaders)
        renderMeshesWithMeshShaders(node, gbufferRTV, depth, scene.materials, scene.cameraIdx, instances);
      else
        renderMeshes(node, gbufferRTV, moti, depth, scene.materials, scene.cameraIdx, scene.prevCameraIdx, instances);
      tasks.addPass(std::move(node));
    }
  }

  // transparent objects...
  // particles draw
  if (rendererOptions.particles && scene.options.particlesDraw)
  {
    auto node = tasks.createPass("particles - draw");
    particleSimulation.render(dev, node, scene.gbufferRTV, scene.depth, cameraArgs);
    tasks.addPass(std::move(node));
  }
}

higanbana::math::float4x4 calculatePerspective(const ActiveCamera& camera, int2 swapchainRes) {
  using namespace higanbana;
  auto aspect = float(swapchainRes.y)/float(swapchainRes.x);
  float4x4 pers = math::perspectivelh(camera.fov, aspect, camera.minZ, camera.maxZ);
  float4x4 rot = math::rotationMatrixLH(camera.direction);
  float4x4 pos = math::translation(camera.position);
  float4x4 perspective = math::mul(pos, math::mul(rot, pers));
  return perspective;
}

void Renderer::ensureViewportCount(int size) {
  if (viewports.size() != static_cast<size_t>(size))
    viewports.resize(size);
}

void Renderer::renderViewports(higanbana::LBS& lbs, higanbana::WTime& time, const RendererOptions& rendererOptions, higanbana::MemView<RenderViewportInfo> viewportsToRender, higanbana::vector<InstanceDraw>& instances, int drawcalls, int drawsSplitInto) {
  if (swapchain.outOfDate())
  {
    dev.adjustSwapchain(swapchain, scdesc);
    resizeExternal(swapchain.buffers().begin()->texture().desc());
  }

  vector<int> indexesToVP;
  for (int i = 0; i < viewportsToRender.size(); ++i) {
    indexesToVP.push_back(i);
  }

  int vpsHandled = 0;
  for (auto& index : indexesToVP) {
    auto& vp = viewports[index];
    auto& vpInfo = viewportsToRender[index];
    auto vpSize = vpInfo.viewportSize;
    if (!rendererOptions.renderImGui)
      vpSize = swapchain.buffers().begin()->texture().desc().desc.size3D().xy();
    vp.resize(dev, vpInfo.viewportSize, vpInfo.options.resolutionScale, swapchain.buffers().begin()->texture().desc().desc.format);
    vpsHandled++;
  }
  for (; vpsHandled < static_cast<int>(viewports.size()); ++vpsHandled) {
    auto& vp = viewports[vpsHandled];
    vp.resize(dev, int2(16, 16), 1.f, swapchain.buffers().begin()->texture().desc().desc.format);
  }

  CommandGraph tasks = dev.createGraph();
  {
    auto ndoe = tasks.createPass("update materials");
    materials.update(dev, ndoe);
    tasks.addPass(std::move(ndoe));
  }
  auto materialArgs = textures.bindlessArgs(dev, materials.srv());

  {
    vector<CameraSettings> sets;
    for (auto& index : indexesToVP) {
      auto& vpInfo = viewportsToRender[index];
      auto& vp = viewports[index];
      auto gbufferRes = vp.gbuffer.desc().desc.size3D().xy();
      vp.perspective = calculatePerspective(vpInfo.camera, gbufferRes);

      auto prevCamera = vp.previousCamera;
      auto newCamera = CameraSettings{ vp.perspective, float4(vpInfo.camera.position, 1.f)};
      vp.previousCamera = newCamera;
      if (vpInfo.options.jitterEnabled){
        prevCamera.perspective = tsaa.jitterProjection(time.getFrame(), gbufferRes, prevCamera.perspective);
        newCamera.perspective = tsaa.jitterProjection(time.getFrame(), gbufferRes, newCamera.perspective);
      }
      vp.previousCameraIndex = static_cast<int>(sets.size());
      sets.push_back(prevCamera);
      vp.currentCameraIndex = static_cast<int>(sets.size());
      sets.push_back(newCamera);
      vp.jitterOffset = tsaa.getJitterOffset(time.getFrame());
    }
    if (!sets.empty()) {
      auto ndoe = tasks.createPass("copy cameras");
      auto matUpdate = dev.dynamicBuffer<CameraSettings>(makeMemView(sets));
      ndoe.copy(cameras, matUpdate);
      tasks.addPass(std::move(ndoe));
    }
  }

  {
    auto node = tasks.createPass("generate Texture");
    genImage.generate(dev, node, proxyTex.uav());
    tasks.addPass(std::move(node));
  }

  if (rendererOptions.particles && rendererOptions.particlesSimulate)
  {
    auto ndoe = tasks.createPass("simulate particles");
    particleSimulation.simulate(dev, ndoe, time.getFrameTimeDelta());
    tasks.addPass(std::move(ndoe));
  }

  vector<CommandNodeVector> nodeVecs;
  for (auto&& vpInfo : viewportsToRender) {
    nodeVecs.push_back(tasks.localThreadVector());
  }
  //for (auto&& vpInfo : viewportsToRender) {
  std::for_each(std::execution::par_unseq, std::begin(indexesToVP), std::end(indexesToVP), [&,materialCopy = materialArgs](int& index) {
    auto& vpInfo = viewportsToRender[index];
    auto& vp = viewports[index];
    auto& options = vpInfo.options;
    auto& localVec = nodeVecs[index];

    Renderer::SceneArguments sceneArgs{vp.gbufferRTV, vp.depthDSV, vp.motionVectorsRTV, materialCopy, options, vp.currentCameraIndex, vp.previousCameraIndex, vp.perspective, vpInfo.camera.position, drawcalls, drawsSplitInto};

    renderScene(localVec, time, rendererOptions, sceneArgs, instances);

    TextureSRV tsaaOutput = vp.gbufferSRV;
    TextureRTV tsaaOutputRTV = vp.gbufferRTV;
    if (options.tsaa)
    {
      auto tsaaNode = localVec.createPass("Temporal Supersampling AA");
      vp.tsaaResolved.next(time.getFrame());
      tsaa.resolve(dev, tsaaNode, vp.tsaaResolved.uav(), renderer::TSAAArguments{vp.jitterOffset, vp.gbufferSRV, vp.tsaaResolved.previousSrv(), vp.motionVectorsSRV, vp.gbufferSRV, vp.tsaaDebugUAV});
      localVec.addPass(std::move(tsaaNode));
      tsaaOutput = vp.tsaaResolved.srv();
      tsaaOutputRTV = vp.tsaaResolved.rtv();
      if (options.tsaaDebug)
        tsaaOutput = vp.tsaaDebugSRV;
    }

    if (options.debugTextures)
    {
      auto node = localVec.createPass("debug textures");
      blitter.beginRenderpass(node, tsaaOutputRTV);
      int2 curPos = int2(0,0);
      int2 target = tsaaOutputRTV.desc().desc.size3D().xy();
      int2 times = int2(target.x/72, target.y/72);
      for (int y = 0; y < times.y; y++) {
        for (int x = 0; x < times.x; x++) {
          curPos = int2(72*x, 72*y);
          auto index = y*times.x + x;
          if (index >= textures.size())
            break;
          blitter.blit(dev, node, tsaaOutputRTV, textures[index], curPos, int2(64,64));
        }
      }
      node.endRenderpass();
      localVec.addPass(std::move(node));
    }
  });
  for (auto&& nodeVec : nodeVecs)
    tasks.addVectorOfPasses(std::move(nodeVec));

  // If you acquire, you must submit it.
  Timer acquireTime;
  std::optional<std::pair<int,TextureRTV>> obackbuffer = dev.acquirePresentableImage(swapchain);
  m_acquireTimeTaken = acquireTime.timeFromLastReset();
  if (!obackbuffer.has_value())
  {
    HIGAN_LOGi( "No backbuffer available...\n");
    dev.submit(tasks);
    return;
  }
  TextureRTV backbuffer = obackbuffer.value().second;
  backbuffer.setOp(LoadOp::Clear);

  {
    auto node = tasks.createPass("acquireImage");
    node.acquirePresentableImage(swapchain);
    tasks.addPass(std::move(node));
  }

  for (auto&& index : indexesToVP) {
    auto& vpInfo = viewportsToRender[index];
    auto& vp = viewports[index];
    auto node = tasks.createPass("tonemapper");
    auto target = backbuffer;
    target = vp.viewportRTV;
    if (!rendererOptions.renderImGui)
      target = backbuffer;
    TextureSRV tsaaOutput = vp.gbufferSRV;
    if (vpInfo.options.tsaa) {
      tsaaOutput = vp.tsaaResolved.srv();
    }
    tonemapper.tonemap(dev, node, target, renderer::TonemapperArguments{tsaaOutput});
    tasks.addPass(std::move(node));
  }

  // IMGUI
  if (rendererOptions.renderImGui)
  {
    auto node = tasks.createPass("IMGui");
    vector<TextureSRV> allViewports;
    for (auto&& viewport : viewports)
      allViewports.push_back(viewport.viewportSRV);
    imgui.render(dev, node, backbuffer, allViewports);
    tasks.addPass(std::move(node));
  }
  
  {
    auto node = tasks.createPass("preparePresent");

    node.prepareForPresent(backbuffer);
    tasks.addPass(std::move(node));
  }

  if (rendererOptions.submitSingleThread)
    dev.submitExperimental(swapchain, tasks, ThreadedSubmission::Sequenced);
  else if (rendererOptions.submitExperimental)
    dev.submitExperimental(swapchain, tasks, ThreadedSubmission::ParallelUnsequenced);
  else if (rendererOptions.submitLBS)
    dev.submitLBS(lbs, swapchain, tasks, ThreadedSubmission::ParallelUnsequenced);
  else
    dev.submit(swapchain, tasks);
    
  {
    HIGAN_CPU_BRACKET("Present");
    dev.present(swapchain, obackbuffer.value().first);
  }
}
}