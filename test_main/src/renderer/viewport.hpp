#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/pingpongTexture.hpp>
#include <higanbana/graphics/common/tiled_image.hpp>
#include <css/low_prio_task.hpp>
#include "camera.hpp"
#include "../raytrace/camera.hpp"
#include "../raytrace/hittable_list.hpp"
#include <higanbana/core/system/time.hpp>

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

  higanbana::Texture    gbufferRaytracing;
  higanbana::TextureSRV gbufferRaytracingSRV;

  higanbana::deque<std::shared_ptr<css::LowPrioTask<size_t>>> workersTiles;
  higanbana::TiledImage cpuRaytrace;
  higanbana::WTime      cpuRaytraceTime;
  size_t nextTileToRaytrace = 0;

  // hmm, misc things
  CameraSettings previousCamera;
  float4x4 perspective;
  int previousCameraIndex;
  int currentCameraIndex;
  int2 jitterOffset;

  rt::Camera prevCam;
  rt::Camera rtCam;
  size_t currentSampleDepth = 1;
  
  rt::HittableList world;
  bool worldChanged;
  
  css::Task<void> resize(higanbana::GpuGroup& device, int2 viewport, float internalScale, higanbana::FormatType backbufferFormat, uint tileSize);
};
}