#pragma once

#include "../world/visual_data_structures.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>

namespace app
{
class WorldRenderer
{
  higanbana::ShaderArgumentsLayout m_pipelineLayout;
  higanbana::GraphicsPipeline m_pipeline;
  higanbana::Renderpass m_renderpass;
public:
  WorldRenderer(higanbana::GpuGroup& device);
  higanbana::ShaderArgumentsLayout shaderLayout() {return m_pipelineLayout;}
  void beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureDSV& depth);
  void endRenderpass(higanbana::CommandGraphNode& node);
  void renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments meshBuffers);
};
}