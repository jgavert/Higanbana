#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/multi_pipeline.hpp>

namespace app::renderer
{
class DepthPyramid
{
  higanbana::ShaderArgumentsLayout m_input;
  higanbana::ComputePipeline pipeline;
  higanbana::Buffer m_counters;
  higanbana::BufferUAV m_countersUAV;
public:
  DepthPyramid(higanbana::GpuGroup& device);
  higanbana::ResourceDescriptor getMinimumDepthPyramidDescriptor(higanbana::TextureSRV depthBuffer);
  void downsample(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureSRV depthBuffer, higanbana::TextureUAV pyramidOutput);
};
}