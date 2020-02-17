#pragma once

#include "../world/visual_data_structures.hpp"
#include "camera.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>

namespace app
{
struct TSAAArguments
{
  higanbana::TextureSRV source;
  higanbana::TextureSRV history;
};
class ShittyTSAA
{
  higanbana::ShaderArgumentsLayout m_staticArgumentsLayout;
  higanbana::ComputePipeline m_pipeline;
  higanbana::Renderpass m_renderpass;
public:
  ShittyTSAA(higanbana::GpuGroup& device);
  void temporalUpsample(higanbana::CommandGraphNode& node, higanbana::TextureUAV output, TSAAArguments args);
};
}