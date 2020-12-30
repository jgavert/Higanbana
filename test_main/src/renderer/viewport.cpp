#include "viewport.hpp"

#include "../raytrace/sphere.hpp"
#include "../raytrace/material.hpp"

namespace app
{
rt::HittableList random_scene() {
  using namespace rt;
  HittableList world;

  auto ground_material = std::make_shared<Lambertian>(double3(0.5, 0.5, 0.5));
  world.add(std::make_shared<Sphere>(double3(0,-1000,0), 1000, ground_material));

  for (int a = -11; a < 11; a++) {
    for (int b = -11; b < 11; b++) {
      auto choose_mat = random_double();
      double3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

      if (length(sub(center, double3(4, 0.2, 0))) > 0.9) {
        std::shared_ptr<Material> sphere_material;

        if (choose_mat < 0.8) {
          // diffuse
          auto albedo = mul(random_vec(), random_vec());
          sphere_material = std::make_shared<Lambertian>(albedo);
          world.add(std::make_shared<Sphere>(center, 0.2, sphere_material));
        } else if (choose_mat < 0.95) {
          // metal
          auto albedo = random_vec(0.5, 1);
          auto fuzz = random_double(0, 0.5);
          sphere_material = std::make_shared<Metal>(albedo, fuzz);
          world.add(std::make_shared<Sphere>(center, 0.2, sphere_material));
        } else {
          // glass
          sphere_material = std::make_shared<Dielectric>(1.5);
          world.add(std::make_shared<Sphere>(center, 0.2, sphere_material));
        }
      }
    }
  }

  auto material1 = std::make_shared<Dielectric>(1.5);
  world.add(std::make_shared<Sphere>(double3(0, 1, 0), 1.0, material1));

  auto material2 = std::make_shared<Lambertian>(double3(0.4, 0.2, 0.1));
  world.add(std::make_shared<Sphere>(double3(-4, 1, 0), 1.0, material2));

  auto material3 = std::make_shared<Metal>(double3(0.7, 0.6, 0.5), 0.0);
  world.add(std::make_shared<Sphere>(double3(4, 1, 0), 1.0, material3));

  return world;
}

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
    //rtCam = rt::Camera(aspect);
    world = random_scene();
    worldChanged = true;
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
    //rtCam = rt::Camera(aspect);
    world = random_scene();
    worldChanged = true;
    nextTileToRaytrace = 0;
  }
  co_return;
}
}