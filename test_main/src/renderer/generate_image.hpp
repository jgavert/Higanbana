#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/time.hpp>

namespace app::renderer
{
class GenerateImage
{
  higanbana::ShaderArgumentsLayout m_argLayout;
  higanbana::ComputePipeline m_pipeline;
  higanbana::WTime time;
  public:
  GenerateImage(higanbana::GpuGroup& device, std::string shaderName, uint3 threadGroups);
  void generate(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureUAV& texture);
};
}