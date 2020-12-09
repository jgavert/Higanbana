#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/multi_pipeline.hpp>

namespace app::renderer
{
class GenerateMips 
{
  higanbana::ShaderArgumentsLayout m_input;
  higanbana::ComputePipeline pipeline;
public:
  GenerateMips(higanbana::GpuGroup& device);
  void generateMipsCS(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::Texture source);
  void generateMipCS(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::TextureSRV sourceMip, higanbana::TextureUAV outputMip);
};
}