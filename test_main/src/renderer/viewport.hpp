#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/pingpongTexture.hpp>
#include "camera.hpp"
namespace app
{
// viewport stores all viewport specific resources
class Viewport
{
  public:
  // imgui texture... when we cannot go straight to backbuffer
  // resources sharing resolution with viewport
  higanbana::Texture    viewport;
  higanbana::TextureSRV viewportSRV;
  higanbana::TextureRTV viewportRTV;
  higanbana::Buffer     sharedViewport; // shared resource from host gpu 0. viewport copied to this.
  higanbana::PingPongTexture tsaaResolved; // tsaa history/current
  higanbana::Texture    tsaaDebug;
  higanbana::TextureSRV tsaaDebugSRV;
  higanbana::TextureUAV tsaaDebugUAV;

  // resources sharing resolution with gbuffer
  higanbana::Texture    mipmaptest;

  higanbana::Texture    gbuffer;
  higanbana::TextureSRV gbufferSRV;
  higanbana::TextureRTV gbufferRTV;
  higanbana::Texture    depth;
  higanbana::TextureDSV depthDSV;
  higanbana::Texture    motionVectors;
  higanbana::TextureSRV motionVectorsSRV;
  higanbana::TextureRTV motionVectorsRTV;

  // hmm, misc things
  CameraSettings previousCamera;
  float4x4 perspective;
  int previousCameraIndex;
  int currentCameraIndex;
  int2 jitterOffset;
  
  void resize(higanbana::GpuGroup& device, int2 viewport, float internalScale, higanbana::FormatType backbufferFormat);
};
}