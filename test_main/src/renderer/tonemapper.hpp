#pragma once

#include "../world/visual_data_structures.hpp"
#include "camera.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>

namespace app::renderer
{
struct TonemapperArguments
{
  higanbana::TextureSRV source;
};
class Tonemapper
{
  higanbana::ShaderArgumentsLayout m_args;
  higanbana::GraphicsPipeline pipelineBGRA;
  higanbana::GraphicsPipeline pipelineRGBA;
  higanbana::Renderpass m_renderpassRGBA;
  higanbana::Renderpass m_renderpassBGRA;
public:
  Tonemapper(higanbana::GpuGroup& device);
  void tonemap(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV output, TonemapperArguments args);
};
}