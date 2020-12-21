#pragma once

#include "../world/visual_data_structures.hpp"
#include "camera.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/multi_pipeline.hpp>

namespace app::renderer
{
struct TonemapperArguments
{
  higanbana::TextureSRV source;
  bool tsaa;
};
class Tonemapper
{
  higanbana::ShaderArgumentsLayout m_args;
  higanbana::GraphicsPipelines<higanbana::FormatType> pipelines;
  higanbana::Renderpasses<higanbana::FormatType> renderpasses;
public:
  Tonemapper(higanbana::GpuGroup& device);
  void tonemap(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV output, TonemapperArguments args);
};
}