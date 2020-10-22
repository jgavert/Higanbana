#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <imgui.h>

#include <higanbana/graphics/helpers/multi_pipeline.hpp>

namespace app::renderer
{
class IMGui
{
  higanbana::ShaderArgumentsLayout vertexLayout;
  higanbana::ShaderArgumentsLayout imageLayout;
  //higanbana::GraphicsPipeline pipeline;
  //higanbana::Renderpass renderpass;
  higanbana::GraphicsPipelines<higanbana::FormatType> pipelines;
  higanbana::Renderpasses<higanbana::FormatType> renderpasses;
  higanbana::Texture fontatlas;
  higanbana::TextureSRV fontatlasSrv;

  higanbana::CpuImage image;
public:
  IMGui(higanbana::GpuGroup& device);
  void render(higanbana::GpuGroup& device, higanbana::CommandGraphNode& graph, higanbana::TextureRTV& target, higanbana::vector<higanbana::TextureSRV>& viewport);
};
}