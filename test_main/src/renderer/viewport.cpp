#include "viewport.hpp"

#include "../raytrace/sphere.hpp"
#include "../raytrace/material.hpp"

namespace app
{
css::Task<void> Viewport::resize(higanbana::GpuGroup& device, int2 targetViewport, float internalScale, higanbana::FormatType backbufferFormat, uint tileSize) {
  using namespace higanbana;
  int2 targetRes = targetViewport; 
  targetRes.x = std::max(targetRes.x, 8);
  targetRes.y = std::max(targetRes.y, 8);
  auto cis = viewport.desc().desc.size3D().xy();
  if (cis.x != targetRes.x || cis.y != targetRes.y || viewport.desc().desc.format != backbufferFormat) {
    viewport = device.createTexture(ResourceDescriptor()
      .setName("viewport for imgui")
      .setFormat(backbufferFormat)
      .setSize(targetRes)
      .setUsage(ResourceUsage::RenderTarget));
    viewportSRV = device.createTextureSRV(viewport);
    viewportRTV = device.createTextureRTV(viewport);
    viewportRTV.setOp(LoadOp::Clear);

    sharedViewport = device.createBuffer(ResourceDescriptor()
      .setName("shared buffer")
      .setFormat(FormatType::Raw32)
      .setCount(sizeFormatSlicePitch(viewport.desc().desc.size3D(), viewport.desc().desc.format) / sizeof(uint32_t))
      .allowCrossAdapter());
    
    tsaaResolved.resize(device, ResourceDescriptor()
      .setSize(targetRes)
      .setFormat(FormatType::Float16RGBA)
      .setUsage(ResourceUsage::RenderTargetRW)
      .setName("tsaa current/history"));

    auto descTsaaResolved = tsaaResolved.desc();
    tsaaDebug = device.createTexture(descTsaaResolved);
    tsaaDebugSRV = device.createTextureSRV(tsaaDebug);
    tsaaDebugUAV = device.createTextureUAV(tsaaDebug);
  }
  int2 currentRes = math::mul(internalScale, float2(targetRes));
  if (currentRes.x > 0 && currentRes.y > 0 && (currentRes.x != gbuffer.desc().desc.size3D().x || currentRes.y != gbuffer.desc().desc.size3D().y))
  {
    auto desc = ResourceDescriptor(gbuffer.desc()).setSize(currentRes);

    mipmaptest = device.createTexture(higanbana::ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(FormatType::Float16RGBA)
      .setMiplevels(higanbana::ResourceDescriptor::AllMips)
      .setUsage(ResourceUsage::RenderTargetRW)
      .setName("mipmaptest"));

    gbuffer = device.createTexture(higanbana::ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(FormatType::Float16RGBA)
      .setUsage(ResourceUsage::RenderTargetRW)
      .setName("gbuffer"));
    gbufferSRV = device.createTextureSRV(gbuffer);
    gbufferRTV = device.createTextureRTV(gbuffer);

    depth = device.createTexture(higanbana::ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(FormatType::Depth32)
      .setUsage(ResourceUsage::DepthStencil)
      .setName("opaqueDepth"));
    depthDSV = device.createTextureDSV(depth);

    motionVectors = device.createTexture(higanbana::ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(FormatType::Float16RGBA)
      .setUsage(ResourceUsage::RenderTarget)
      .setName("motion vectors"));
    motionVectorsSRV = device.createTextureSRV(motionVectors);
    motionVectorsRTV = device.createTextureRTV(motionVectors);

    // rt weekend
    gbufferRaytracing = device.createTexture(higanbana::ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(FormatType::Float32RGBA)
      .setUsage(ResourceUsage::GpuReadOnly)
      .setName("gbuffer cpu raytracing"));
    gbufferRaytracingSRV = device.createTextureSRV(gbufferRaytracing);

    while(!workersTiles.empty()) {
      auto& work = *workersTiles.front();
      co_await work;
      HIGAN_ASSERT(work.is_ready(), "lol");
      workersTiles.pop_front();
    }
    cpuRaytrace = TiledImage(desc.desc.size3D().xy(), uint2(tileSize, tileSize), FormatType::Float32RGBA);
    double aspect = double(desc.desc.size3D().x) / double(desc.desc.size3D().y);
    rtCam = rt::Camera(aspect);
    world.clear();
    auto material_ground = std::make_shared<rt::Lambertian>(double3(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<rt::Lambertian>(double3(0.1, 0.2, 0.5));
    auto material_left   = std::make_shared<rt::Dielectric>(1.5);
    auto material_right  = std::make_shared<rt::Metal>(double3(0.8, 0.6, 0.2), 1.0);

    world.add(std::make_shared<rt::Sphere>(double3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(std::make_shared<rt::Sphere>(double3( 0.0,    0.0, -1.0),   0.5, material_center));
    world.add(std::make_shared<rt::Sphere>(double3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(std::make_shared<rt::Sphere>(double3(-1.0,    0.0, -1.0),  -0.4, material_left));
    world.add(std::make_shared<rt::Sphere>(double3( 1.0,    0.0, -1.0),   0.5, material_right));
    nextTileToRaytrace = 0;
  }
  if (cpuRaytrace.tileSize().x != tileSize) {
    while(!workersTiles.empty()) {
      auto& work = *workersTiles.front();
      co_await work;
      HIGAN_ASSERT(work.is_ready(), "lol");
      workersTiles.pop_front();
    }
    cpuRaytrace = TiledImage(currentRes, uint2(tileSize, tileSize), FormatType::Float32RGBA);
    double aspect = double(currentRes.x) / double(currentRes.y);
    rtCam = rt::Camera(aspect);
    world.clear();
    auto material_ground = std::make_shared<rt::Lambertian>(double3(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<rt::Lambertian>(double3(0.1, 0.2, 0.5));
    auto material_left   = std::make_shared<rt::Dielectric>(1.5);
    auto material_right  = std::make_shared<rt::Metal>(double3(0.8, 0.6, 0.2), 1.0);

    world.add(std::make_shared<rt::Sphere>(double3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(std::make_shared<rt::Sphere>(double3( 0.0,    0.0, -1.0),   0.5, material_center));
    world.add(std::make_shared<rt::Sphere>(double3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(std::make_shared<rt::Sphere>(double3(-1.0,    0.0, -1.0),  -0.4, material_left));
    world.add(std::make_shared<rt::Sphere>(double3( 1.0,    0.0, -1.0),   0.5, material_right));
    nextTileToRaytrace = 0;
  }
  co_return;
}
}