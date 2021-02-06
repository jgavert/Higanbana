#include "rendering.hpp"

#include <higanbana/core/profiling/profiling.hpp>
#include <higanbana/graphics/common/image_loaders.hpp>
#include <higanbana/core/ranges/rectangle.hpp>

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
  , blockRend(dev, m_camerasLayout, textures.bindlessLayout())
  , tsaa(dev)
  , tonemapper(dev)
  , blitter(dev)
  , mipper(dev)
  , depthPyramid(dev)
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
void Renderer::loadLogos(higanbana::FileSystem& fs) {
  if (!fs.fileExists("/misc/dx12u_logo.png") || !fs.fileExists("/misc/vulkan_logo.png"))
    return;
  
  // load dx12
  auto dx_logo = textureUtils::loadImageFromFilesystem(fs, "/misc/dx12u_logo.png", false);
  auto vk_logo = textureUtils::loadImageFromFilesystem(fs, "/misc/vulkan_logo.png", false);
  m_logoDx12 = dev.createTexture(dx_logo);
  m_logoDx12Srv = dev.createTextureSRV(m_logoDx12);
  m_logoVulkan = dev.createTexture(vk_logo);
  m_logoVulkanSrv = dev.createTextureSRV(m_logoVulkan);
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

void Renderer::testMipper(higanbana::CommandNodeVector& tasks, higanbana::Texture testTexture, higanbana::TextureRTV& backbuffer, higanbana::Texture someData) {
  {
    auto node = tasks.createPass("copy backbuffer as example", QueueType::Graphics);
    node.copy(testTexture, Subresource().mip(0).slice(0), int3(0,0,0), someData, Subresource().mip(0).slice(0), Box(uint3(0,0,0), someData.size3D()));
    tasks.addPass(std::move(node));
  }
  {
    auto node = tasks.createPass("generate mips", QueueType::Graphics);
    mipper.generateMipsCS(dev, node, testTexture);
    tasks.addPass(std::move(node));
  }
  {
    auto node = tasks.createPass("output mips");
    auto mips = testTexture.desc().desc.miplevels;
    blitter.beginRenderpass(node, backbuffer);
    int offset = 0;
    auto startSize = backbuffer.size3D().x;
    for (auto mip = 0; mip < mips; ++ mip) {
      auto srv = dev.createTextureSRV(testTexture, ShaderViewDescriptor().setMostDetailedMip(mip).setMipLevels(1));
      blitter.blitScale(dev, node, backbuffer, srv, int2(offset, 0), 0.5f);
      startSize = startSize / 2;
      offset += startSize;
    }
    node.endRenderpass();
    tasks.addPass(std::move(node));
  }
}

css::Task<void> Renderer::renderScene(higanbana::CommandNodeVector& tasks, higanbana::WTime& time, const RendererOptions& rendererOptions, const Renderer::SceneArguments scene, higanbana::vector<InstanceDraw>& instances, higanbana::vector<ChunkBlockDraw>& blocks) {
  HIGAN_CPU_BRACKET("renderScene");
  {
    auto node = tasks.createPass("composite", QueueType::Graphics, scene.options.gpuToUse);
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
    if (instances.empty() && blocks.empty())
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
        //HIGAN_LOGi("%d / %d = %d\n", scene.drawcalls, scene.drawsSplitInto, stepSize);
        for (int i = 0; i < scene.drawcalls; i+=stepSize)
        {
          if (i+stepSize > scene.drawcalls)
          {
            stepSize = stepSize - (i+stepSize - scene.drawcalls);
          }
          nodes.push_back(std::make_tuple(tasks.createPass("opaquePass - cubes", QueueType::Graphics, scene.options.gpuToUse), i, stepSize));
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
          nodes.push_back(std::make_tuple(tasks.createPass("opaquePass - cubes", QueueType::Graphics, scene.options.gpuToUse), cubesDrawn, reverseDraw[i]));
          cubesDrawn += reverseDraw[i];
        } 
      }
      std::unique_ptr<vector<css::Task<void>>> passes = std::make_unique<vector<css::Task<void>>>();
      for (auto& node : nodes) {
        HIGAN_CPU_BRACKET("draw cubes - inner pass");
        auto ldepth = depth;
        if (std::get<1>(node) == 0)
          ldepth.setOp(LoadOp::Clear);
        else
          ldepth.setOp(LoadOp::Load);
        
        passes->emplace_back(cubes.oldOpaquePass2(dev, time.getFTime(), std::get<0>(node), scene.perspective, gbufferRTV, ldepth, ind, args, scene.drawcalls, std::get<1>(node),  std::get<1>(node)+std::get<2>(node)));
      }
      int passId = 0;
      for (auto& pass : *passes) {
        if (!pass.is_ready())
          co_await pass;
        
        tasks.addPass(std::move(std::get<0>(nodes[passId])));
        passId++;
      }
    }
    else if (!instances.empty())
    {
      HIGAN_CPU_BRACKET("opaquePass - ecs");
      auto node = tasks.createPass("opaquePass - ecs", QueueType::Graphics, scene.options.gpuToUse);
      auto gbufferRTV = scene.gbufferRTV;
      auto depth = scene.depth;
      auto moti = scene.motionVectors;
      HIGAN_ASSERT(scene.materials, "wtf!");
      moti.clearOp(float4(0.f,0.f,0.f,0.f));
      if (rendererOptions.allowMeshShaders && scene.options.useMeshShaders)
        renderMeshesWithMeshShaders(node, gbufferRTV, depth, scene.materials, scene.cameraIdx, instances);
      else
        renderMeshes(node, gbufferRTV, moti, depth, scene.materials, scene.cameraIdx, scene.prevCameraIdx, instances);
      tasks.addPass(std::move(node));
    }
    else{
      HIGAN_CPU_BRACKET("opaquePass - blocks");
      auto node = tasks.createPass("opaquePass - blocks", QueueType::Graphics, scene.options.gpuToUse);
      auto gbufferRTV = scene.gbufferRTV;
      auto depth = scene.depth;
      auto moti = scene.motionVectors;
      HIGAN_ASSERT(scene.materials, "wtf!");
      moti.clearOp(float4(0.f,0.f,0.f,0.f));
      blockRend.renderBlocks(dev, node, gbufferRTV, moti, depth, cameraArgs, scene.materials, scene.cameraIdx, scene.prevCameraIdx, blocks);
      tasks.addPass(std::move(node));
      
    }
  }

  // transparent objects...
  // particles draw
  if (rendererOptions.particles && scene.options.particlesDraw)
  {
    auto node = tasks.createPass("particles - draw", QueueType::Graphics, scene.options.gpuToUse);
    particleSimulation.render(dev, node, scene.gbufferRTV, scene.depth, cameraArgs);
    tasks.addPass(std::move(node));
  }
  co_return;
}

higanbana::math::float4x4 calculatePerspective(const ActiveCamera& camera, int2 swapchainRes) {
  using namespace higanbana;
  auto aspect = float(swapchainRes.y)/float(swapchainRes.x);
  float4x4 pers = math::perspectiveLHInverseZ(camera.fov, aspect, camera.minZ, camera.maxZ);
  float4x4 rot = math::rotationMatrixLH(math::normalize(camera.direction));
  float4x4 pos = math::translation(camera.position);
  float4x4 perspective = math::mul(pos, math::mul(rot, pers));
  return perspective;
}

css::Task<void> Renderer::ensureViewportCount(int size) {
  if (viewports.size() > static_cast<size_t>(size))
  {
    for (size_t iter = static_cast<size_t>(size); iter < viewports.size(); iter++) {
      for (auto&& tile : viewports[iter].workersTiles) {
        co_await *tile;
      }
    }
  }
  if (viewports.size() != static_cast<size_t>(size))
    viewports.resize(size);
}

void Renderer::handleReadbacks(higanbana::FileSystem& fs) {
  while (!readbacks.empty()) {
    if (readbacks.front().readback.ready()){
      auto rb = readbacks.front().readback.get();
      auto v = rb.view<uint8_t>();
      HIGAN_LOGi("Something was readback %d bytes\n", v.size());
      auto desc = readbacks.front().tex;
      CpuImage image(desc);
      auto subRes = image.subresource(0,0);
      auto dstRowPitch = subRes.rowPitch();
      auto srcRowPitch = sizeFormatRowPitch(desc.size(), desc.desc.format);
      for (size_t y = 0; y < desc.size().y; ++y) {
        auto dstOffset = y*dstRowPitch;
        auto srcOffset = y*srcRowPitch;
        memcpy(subRes.data() + dstOffset, v.data() + srcOffset, dstRowPitch);
      }
      textureUtils::saveImageFromFilesystemPNG(fs, readbacks.front().filepath, image);
      readbacks.pop_front();
    }
    else{
      break;
    }
  }
}

size_t Renderer::raytraceSampleCount(int viewportIdx) {
  if (viewportIdx < 0)
    return 0;
  if (viewportIdx >= viewports.size())
    return 0;
  return *viewports[viewportIdx].cpuRaytrace.tileRemap(0).iterations;
}

// seconds per iteration
double Renderer::raytraceSecondsPerIteration(int viewportIdx) {
  if (viewportIdx < 0)
    return 0;
  if (viewportIdx >= viewports.size())
    return 0;
  return viewports[viewportIdx].cpuRaytraceTime.getAverageFps();
}

css::Task<void> Renderer::renderViewports(higanbana::LBS& lbs, higanbana::WTime time, const RendererOptions rendererOptions, higanbana::MemView<RenderViewportInfo> viewportsToRender, rt::World& rtworld, higanbana::vector<InstanceDraw>& instances, higanbana::vector<ChunkBlockDraw>& blocks, int drawcalls, int drawsSplitInto) {

  bool adjustSwapchain = false;
  if (rendererOptions.enableHDR == true && scdesc.desc.colorSpace != Colorspace::BT2020)
  {
    adjustSwapchain = true;
    scdesc = scdesc.formatType(FormatType::Unorm10RGB2A).colorspace(Colorspace::BT2020);
  } else if (rendererOptions.enableHDR == false && scdesc.desc.colorSpace != Colorspace::BT709) {
    adjustSwapchain = true;
    scdesc = scdesc.formatType(FormatType::Unorm8RGBA).colorspace(Colorspace::BT709);
  }
  if (swapchain.outOfDate() || adjustSwapchain)
  {
    if (presentTask.get() != nullptr) {
      co_await *presentTask;
      presentTask.reset();
    }
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
    if (!rendererOptions.renderImGui) {
      vpInfo.viewportSize = swapchain.buffers().begin()->texture().desc().desc.size3D().xy();
    }
    auto format = swapchain.buffers().begin()->texture().desc().desc.format;
    co_await vp.resize(dev, vpInfo.viewportSize, vpInfo.options.resolutionScale, format, vpInfo.options.tileSize);
    vpsHandled++;
  }
  for (; vpsHandled < static_cast<int>(viewports.size()); ++vpsHandled) {
    auto& vp = viewports[vpsHandled];
    co_await vp.resize(dev, int2(16, 16), 1.f, swapchain.buffers().begin()->texture().desc().desc.format, 32);
  }

  CommandGraph tasks = dev.createGraph();
  for (int i = 0; i < dev.deviceCount(); i++)
  {
    auto ndoe = tasks.createPass("update materials", QueueType::Graphics, i);
    materials.update(dev, ndoe);
    tasks.addPass(std::move(ndoe));
  }
  materials.allUpdated();
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
      auto frame = time.getFrame();
      if (vpInfo.options.jitterEnabled && vpInfo.options.tsaa){
        prevCamera.perspective = tsaa.jitterProjection(frame, gbufferRes, prevCamera.perspective);
        newCamera.perspective = tsaa.jitterProjection(frame, gbufferRes, newCamera.perspective);
      }
      vp.previousCameraIndex = static_cast<int>(sets.size());
      sets.push_back(prevCamera);
      vp.currentCameraIndex = static_cast<int>(sets.size());
      sets.push_back(newCamera);
      vp.jitterOffset = tsaa.getJitterOffset(frame);
      vp.perspective = newCamera.perspective;

      float3 dir = math::normalize(rotateVector({ 0.f, 0.f, -1.f }, vpInfo.camera.direction));
      float3 updir = math::normalize(rotateVector({ 0.f, 1.f, 0.f }, vpInfo.camera.direction));
      float3 sidedir = math::normalize(rotateVector({ 1.f, 0.f, 0.f }, vpInfo.camera.direction));
      double aspectRatio = double(gbufferRes.x) / double(gbufferRes.y);
      vp.prevCam = vp.rtCam;
      vp.rtCam = rt::Camera(vpInfo.camera.position, dir, updir, sidedir, vpInfo.camera.fov, aspectRatio, vpInfo.camera.aperture, vpInfo.camera.focusDist);
    }
    if (!sets.empty()) {
      auto matUpdate = dev.dynamicBuffer<CameraSettings>(makeMemView(sets));
      for (int i = 0; i < dev.deviceCount(); i++) {
        auto ndoe = tasks.createPass("copy cameras", QueueType::Graphics, i);
        ndoe.copy(cameras, matUpdate);
        tasks.addPass(std::move(ndoe));
      }
    }
  }

  for (int i = 0; i < dev.deviceCount(); i++) {
    auto node = tasks.createPass("generate Texture", QueueType::Graphics, i);
    genImage.generate(dev, node, proxyTex.uav());
    tasks.addPass(std::move(node));
  }

  for (int i = 0; i < dev.deviceCount(); i++)
    if (rendererOptions.particles && rendererOptions.particlesSimulate)
    {
      auto ndoe = tasks.createPass("simulate particles", QueueType::Graphics, i);
      particleSimulation.simulate(dev, ndoe, time.getFrameTimeDelta());
      tasks.addPass(std::move(ndoe));
    }


  vector<CommandNodeVector> nodeVecs;
  for (auto&& vpInfo : viewportsToRender) {
    nodeVecs.push_back(tasks.localThreadVector());
  }

  // reset viewports rt world
  for (auto&& vp : viewportsToRender) {
    if (rtworld.worldChanged){
      vp.options.worldChanged = true;
    }
  }
  rtworld.worldChanged = false;

  vector<css::Task<void>> sceneTasks;
  for (auto&& index : indexesToVP) {
    auto& vpInfo = viewportsToRender[index];
    auto& vp = viewports[index];
    auto& options = vpInfo.options;
    auto& localVec = nodeVecs[index];

    if (!vpInfo.options.useRaytracing) {
      Renderer::SceneArguments sceneArgs{vp.gbufferRTV, vp.depthDSV, vp.motionVectorsRTV, materialArgs, options, vp.currentCameraIndex, vp.previousCameraIndex, vp.perspective, vpInfo.camera.position, drawcalls, drawsSplitInto};

      sceneTasks.emplace_back(renderScene(localVec, time, rendererOptions, sceneArgs, instances, blocks));
    } else {
      // raytrace first
      auto& tiles = vp.workersTiles;
      // new
      size_t activeNodes = tiles.size();
      {
        if (vp.rtCam != vp.prevCam || vpInfo.options.worldChanged || vp.currentSampleDepth != vpInfo.options.sampleDepth) {
          if (false && !vpInfo.options.rtIncremental) {
            auto node = localVec.createPass("clear rt texture", QueueType::Graphics, options.gpuToUse);
            //HIGAN_LOGi("camera was different!\n");
            //node.clear(vp.gbufferRaytracing);
            vector<float4> empty;
            for (size_t i = 0; i < 64*64; ++i) {
              empty.push_back(float4(0.f));
            }
            auto dyn = dev.dynamicImage(makeByteView<float4>(empty.data(), empty.size() * sizeof(float4)), sizeof(float4) * 64);
            for (auto tile : ranges::Range2D(vp.gbufferRaytracing.size3D().xy(), {64, 64})) {
              node.copy(vp.gbufferRaytracing, Subresource(), uint3(tile.leftTop, 0), dyn, Box(uint3(0,0,0), uint3(tile.size(), 1)));
            }
            localVec.addPass(std::move(node));
          }
          vp.nextTileToRaytrace = 0;
          for (auto&& tile : tiles) {
            co_await *tile;
          }
          for (int tileCount = 0; tileCount < vp.cpuRaytrace.size(); tileCount++) {
            *vp.cpuRaytrace.tile(tileCount).iterations = 0;
          }
          vp.cpuRaytraceTime.firstTick();
          vp.currentSampleDepth = vpInfo.options.sampleDepth;
          vpInfo.options.worldChanged = false;
        }
        int readyTiles = 0;
        for (auto&& tile : tiles) {
          if (tile->is_ready()){
            readyTiles++;
            continue;
          }
          break;
        }
        
        if (readyTiles > 0 || vpInfo.options.raytraceRealtime) {
          auto node = localVec.createPass("copy raytracing to gpu", QueueType::Graphics, options.gpuToUse);
          while (!tiles.empty()) {
            if (!vpInfo.options.raytraceRealtime) {
              if (!tiles.front()->is_ready())
                break;
            } else {
              co_await *tiles.front();
            }
            auto tileIdx = tiles.front()->get();
            auto tile = vp.cpuRaytrace.tileRemap(tileIdx);
            if (tileIdx == vp.cpuRaytrace.size()-1) {
              vp.cpuRaytraceTime.tick();
            }
            auto dyn = dev.dynamicImage(tile.pixels, sizeof(float4) * tile.size.x);
            node.copy(vp.gbufferRaytracing, Subresource(), uint3(tile.offset, 0), dyn, Box(uint3(0,0,0), uint3(tile.size, 1)));
            //HIGAN_LOGi("%ux%u %ux%u\n", tile.offset.x, tile.offset.y, tile.size.x, tile.size.y);
            tiles.pop_front();
            activeNodes--;
          }
          localVec.addPass(std::move(node));
        }
      }

      size_t max_tiles_compute = std::min(static_cast<size_t>(vpInfo.options.tilesToComputePerFrame), vp.cpuRaytrace.size());
      size_t tiles_to_compute = 0;
      if (max_tiles_compute > activeNodes)
        tiles_to_compute = max_tiles_compute - activeNodes;
      if (vpInfo.options.raytraceRealtime) {
        tiles_to_compute = vp.cpuRaytrace.size();
        vp.nextTileToRaytrace = 0;
      }
      auto tileSize = vpInfo.options.tileSize;
      auto samplesPerPixel = vpInfo.options.samplesPerPixel;
      auto sampleDepth = vpInfo.options.sampleDepth;
      if (vpInfo.options.raytraceRealtime) {
        samplesPerPixel = std::min(1, samplesPerPixel);
        sampleDepth = std::min(4, sampleDepth);
      }
      //HIGAN_LOGi("%d: %dx%d vs %dx%d\n", index, vpInfo.viewportSize.x, vpInfo.viewportSize.y, vp.cpuRaytrace.size2D().x, vp.cpuRaytrace.size2D().y);
      for (int tileCount = 0; tileCount < tiles_to_compute; tileCount++) {
        auto tileIdx = vp.nextTileToRaytrace % vp.cpuRaytrace.size();
        //HIGAN_LOGi("index -> %d ", tileIdx);
        higanbana::TileView tilev;
        if (vpInfo.options.raytraceRealtime)
          tilev = vp.cpuRaytrace.tile(tileIdx);
        else
          tilev = vp.cpuRaytrace.tileRemap(tileIdx);
        vp.nextTileToRaytrace = (vp.nextTileToRaytrace +1) % vp.cpuRaytrace.size();

        auto tileTask = [&](TileView tile, float time, double2 vpsize, size_t tileIdx, int samples, int sampleDepth, rt::Camera rtCam, rt::HittableList& list, bool incremental) -> css::LowPrioTask<size_t> {
          double2 offset = double2(tile.offset);
          size_t iterations = *tile.iterations;
          auto total = double(samples+iterations);
          auto oldMul = double(iterations) / total;
          auto newMul = double(samples) / total;
          for (size_t y = 0; y < tile.size.y; y++) {
            for (size_t x = 0; x < tile.size.x; x++) {
              double3 pixel = double3(0.0);
              for (size_t sample = 0; sample < samples; sample++) {
                auto uvv = div(add(double2(x+rt::random_double(),y+rt::random_double()), offset), vpsize);
                double2 uv = double2(uvv.x, 1.0-uvv.y);
                // Calculate direction
                auto ray = rtCam.get_ray(uv);
                // color
                auto color = rtCam.ray_color(ray, list, sampleDepth);
                pixel = add(pixel, double3(color.x, color.y, color.z));
              }
              pixel = rt::color_samples(pixel, samples);
              if (incremental) {
                auto oldPixel = double3(tile.load<float4>(uint2(x, y)).xyz());
                pixel = add(mul(oldPixel, oldMul), mul(pixel, newMul));
              }
              tile.save<float4>(uint2(x, y), float4(pixel, 1.0f));
              //HIGAN_LOGi("%.3f %.3f %.3f\n", color.x, color.y, color.z);
              //tile.save<float4>(uint2(x, y), float4(uv.x, uv.y, 0.25f, 1.f));
            }
          }
          if (incremental) {
            *tile.iterations += samples;
          } else
            *tile.iterations = samples;

          co_return tileIdx;
        };

        tiles.push_back(std::make_shared<css::LowPrioTask<size_t>>(tileTask(tilev, time.getFTime(), sub(double2(vp.gbufferRaytracing.size3D().xy()), double2(-1.0, -1.0)), tileIdx, samplesPerPixel, sampleDepth, vp.rtCam, rtworld.world, vpInfo.options.rtIncremental)));
      }

      {
        auto node = localVec.createPass("blit raytracing to gbuffer", QueueType::Graphics, options.gpuToUse);
        blitter.beginRenderpass(node, vp.gbufferRTV);
        blitter.blitImage(dev, node, vp.gbufferRTV, vp.gbufferRaytracingSRV, app::renderer::Blitter::FitMode::Fill);
        node.endRenderpass();
        localVec.addPass(std::move(node));
      }
    }
  }

  //co_await sceneTasks.back();
  for (auto& task : sceneTasks) {
    if (!task.is_ready())
      co_await task;
  }

  for (auto&& index : indexesToVP) {
    auto& vpInfo = viewportsToRender[index];
    auto& vp = viewports[index];
    auto& options = vpInfo.options;
    auto& localVec = nodeVecs[index];
    if (!vpInfo.options.useRaytracing){
      TextureSRV tsaaOutput = vp.gbufferSRV;
      TextureRTV tsaaOutputRTV = vp.gbufferRTV;
      if (options.tsaa)
      {
        auto tsaaNode = localVec.createPass("Temporal Supersampling AA", QueueType::Graphics, options.gpuToUse);
        vp.tsaaResolved.next(time.getFrame());
        auto motionVectors = instances.empty() ? TextureSRV() : vp.motionVectorsSRV;
        tsaa.resolve(dev, tsaaNode, vp.tsaaResolved.uav(), renderer::TSAAArguments{vp.jitterOffset, vp.gbufferSRV, vp.tsaaResolved.previousSrv(), motionVectors, vp.gbufferSRV, vp.tsaaDebugUAV});
        localVec.addPass(std::move(tsaaNode));
        tsaaOutput = vp.tsaaResolved.srv();
        tsaaOutputRTV = vp.tsaaResolved.rtv();
      }

      if (options.debugTextures)
      {
        auto node = localVec.createPass("debug textures", QueueType::Graphics, options.gpuToUse);
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
    }
  }
  for (auto&& nodeVec : nodeVecs)
    tasks.addVectorOfPasses(std::move(nodeVec));

  // If you acquire, you must submit it.
  Timer acquireTime;
  if (presentTask.get() != nullptr) {
    co_await *presentTask;
    presentTask.reset();
  }
  std::optional<std::pair<int,TextureRTV>> obackbuffer = dev.acquirePresentableImage(swapchain);
  m_acquireTimeTaken = acquireTime.timeFromLastReset();
  if (!obackbuffer.has_value())
  {
    HIGAN_LOGi( "No backbuffer available...\n");
    dev.submit(tasks);
    co_return;
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
    auto target = backbuffer;
    target = vp.viewportRTV;
    if (!rendererOptions.renderImGui && vpInfo.options.gpuToUse == 0)
      target = backbuffer;
    TextureSRV tsaaOutput = vp.gbufferSRV;
    if (vpInfo.options.tsaa && !vpInfo.options.useRaytracing)
      tsaaOutput = vp.tsaaResolved.srv();
    if (vpInfo.options.tsaaDebug)
      tsaaOutput = vp.tsaaDebugSRV;

    {
      auto node = tasks.createPass("tonemapper", QueueType::Graphics, vpInfo.options.gpuToUse);
      tonemapper.tonemap(dev, node, target, renderer::TonemapperArguments{tsaaOutput, vpInfo.options.tsaa && !vpInfo.options.useRaytracing});
      tasks.addPass(std::move(node));
    }

    auto logo = m_logoDx12Srv;
    if (dev.activeDevicesInfo()[vpInfo.options.gpuToUse].api == GraphicsApi::Vulkan)
      logo = m_logoVulkanSrv;
    
    if (logo.texture().handle() && false)
    {
      auto node = tasks.createPass("drawLogo", QueueType::Graphics, vpInfo.options.gpuToUse);
      blitter.beginRenderpass(node, target);
      blitter.blitScale(dev, node, target, logo, int2(10,20), 0.10f);
      node.endRenderpass();
      tasks.addPass(std::move(node));
    }
  }

  // send from other gpu's to main gpu
  for (auto&& index : indexesToVP) {
    auto& vpInfo = viewportsToRender[index];
    auto& vp = viewports[index];
    if (vpInfo.options.gpuToUse != 0) { // I cannot read the condition, needs better naming for variable.
      auto node = tasks.createPass("shared gpu transfer", QueueType::Graphics, vpInfo.options.gpuToUse);
      node.copy(vp.sharedViewport, size_t(0), vp.viewport, Subresource().mip(0).slice(0));
      tasks.addPass(std::move(node));
    }
  }
  // copy all received to samplable structure.
  for (auto&& index : indexesToVP) {
    auto& vpInfo = viewportsToRender[index];
    auto& vp = viewports[index];
    if (vpInfo.options.gpuToUse != 0) { // I cannot read the condition, needs better naming for variable.
      auto node = tasks.createPass("receiving transfer", QueueType::Graphics, 0);
      node.copy(vp.viewport, Subresource().mip(0).slice(0), vp.sharedViewport, 0);
      tasks.addPass(std::move(node));
    }
  }

  if (rendererOptions.testMipper)
  {
    auto node = tasks.localThreadVector();
    for (auto&& viewport : viewports) {
      testMipper(node, viewport.mipmaptest, viewport.viewportRTV, viewport.gbuffer);
    }
    tasks.addVectorOfPasses(std::move(node));
  }

  // screenshot
  {
    for (auto&& index : indexesToVP) {
      auto& vpInfo = viewportsToRender[index];
      auto& vp = viewports[index];
      if (vpInfo.screenshot) {
        auto node = tasks.createPass("screenshot");
        auto rb = node.readback(vp.viewport);
        std::string filename = "/test";
        filename += std::to_string(index);
        filename += ".png";
        readbacks.push_back(ReadbackTexture{filename, vp.viewport.desc(), rb});
        tasks.addPass(std::move(node));
      }
    }
  }

  // IMGUI
  if (rendererOptions.renderImGui)
  {
    auto node = tasks.createPass("IMGui");
    vector<TextureSRV> allViewports;
    allViewports.resize(viewports.size());
    int i = 0;
    for (auto&& vp : viewports) {
      allViewports[i] = vp.viewportSRV;
      i++;
    }
    imgui.render(dev, node, backbuffer, allViewports);
    tasks.addPass(std::move(node));
  }
  
  {
    auto node = tasks.createPass("preparePresent");
    node.prepareForPresent(backbuffer);
    tasks.addPass(std::move(node));
  }

  if (rendererOptions.submitSingleThread)
    dev.submit(swapchain, tasks, ThreadedSubmission::Sequenced);
  else if (rendererOptions.submitExperimental)
    co_await dev.asyncSubmit(swapchain, tasks);
  else
    dev.submit(swapchain, tasks, ThreadedSubmission::Parallel);
    
  {
    HIGAN_CPU_BRACKET("Present");
    
    if (rendererOptions.submitExperimental && !rendererOptions.submitSingleThread)
      presentTask = std::make_unique<css::Task<void>>(dev.asyncPresent(swapchain, obackbuffer.value().first));
    else
      dev.present(swapchain, obackbuffer.value().first);
  }
  co_return;
}
css::Task<void> Renderer::cleanup() {
  for (auto& vp : viewports) {
    while(!vp.workersTiles.empty()) {
      co_await *vp.workersTiles.front();
      vp.workersTiles.pop_front();
    }
  }
  if (presentTask.get() != nullptr) {
    co_await *presentTask;
    presentTask.reset();
  }
}
}