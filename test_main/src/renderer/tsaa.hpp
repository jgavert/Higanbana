#pragma once

#include "../world/visual_data_structures.hpp"
#include "camera.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/math/math.hpp>

namespace app::renderer
{
struct TSAAArguments
{
  higanbana::TextureSRV source;
  higanbana::TextureSRV history;
  higanbana::TextureSRV motionVectors;
  higanbana::TextureSRV groundtruth;
  higanbana::TextureUAV debug;
};
class ShittyTSAA
{
  higanbana::ShaderArgumentsLayout m_args;
  higanbana::ComputePipeline m_pipeline;
  // Sub-sample positions for 16x TAA
  const int2 SampleLocations16[16] = {
    int2(-8, 0),
    int2(-6, -4),
    int2(-3, -2),
    int2(-2, -6),
    int2(1, -1),
    int2(2, -5),
    int2(6, -7),
    int2(5, -3),
    int2(4, 1),
    int2(7, 4),
    int2(3, 5),
    int2(0, 7),
    int2(-1, 3),
    int2(-4, 6),
    int2(-7, 8),
    int2(-5, 2)};

  // Sub-sample positions for 8x TAA
  const int2 SampleLocations8[8] = {
    int2(-7, 1),
    int2(-5, -5),
    int2(-1, -3),
    int2(3, -7),
    int2(5, -1),
    int2(7, 7),
    int2(1, 3),
    int2(-3, 5)};
  int2 currentJitter;
public:
  ShittyTSAA(higanbana::GpuGroup& device);
  float4x4 jitterProjection(int frame, int2 outputsize, float4x4 projection);
  void resolve(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureUAV output, TSAAArguments args);
};
}