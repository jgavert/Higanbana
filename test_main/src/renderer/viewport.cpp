#include "viewport.hpp"

namespace app
{
void Viewport::resize(higanbana::GpuGroup& device, int2 targetViewport, float internalScale, higanbana::FormatType backbufferFormat) {
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

    gbufferRaytracing = device.createTexture(higanbana::ResourceDescriptor()
      .setSize(desc.desc.size3D())
      .setFormat(FormatType::Float32RGBA)
      .setUsage(ResourceUsage::GpuReadOnly)
      .setName("gbuffer cpu raytracing"));
    gbufferRaytracingSRV = device.createTextureSRV(gbufferRaytracing);

    cpuRaytrace = TiledImage(desc.desc.size3D().xy(), uint2(32, 32), FormatType::Float32RGBA);
    double aspect = desc.desc.size3D().x / desc.desc.size3D().y;
    rtCam = rt::Camera(aspect);
  }
}
}