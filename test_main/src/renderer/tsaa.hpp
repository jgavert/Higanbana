#pragma once

#include "../world/visual_data_structures.hpp"
#include "camera.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>

namespace app::renderer
{
struct TSAAArguments
{
  higanbana::TextureSRV source;
  higanbana::TextureSRV history;
};
class ShittyTSAA
{
  higanbana::ShaderArgumentsLayout m_args;
  higanbana::ComputePipeline m_pipeline;
public:
  ShittyTSAA(higanbana::GpuGroup& device);
  void resolve(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureUAV output, TSAAArguments args);
};
}