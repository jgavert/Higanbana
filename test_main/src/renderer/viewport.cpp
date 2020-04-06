#include "viewport.hpp"

namespace app
{
void Viewport::resize(higanbana::GpuGroup& device, int2 targetViewport, float internalScale, higanbana::FormatType backbufferFormat) {
  using namespace higanbana;
  int2 targetRes = targetViewport; 
  targetRes.x = std::max(targetRes.x, 8);
  targetRes.y = std::max(targetRes.y, 8);
  auto cis = viewport.desc().desc.size3D().xy();
  if (cis.x != targetRes.x || cis.y != targetRes.y) {
    viewport = device.createTexture(ResourceDescriptor()
      .setName("viewport for imgui")
      .setFormat(backbufferFormat)
      .setSize(targetRes)
      .setUsage(ResourceUsage::RenderTarget));
    viewportSRV = device.createTextureSRV(viewport);
    viewportRTV = device.createTextureRTV(viewport);
    viewportRTV.setOp(LoadOp::Clear);
    
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
  }
}
}