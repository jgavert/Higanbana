#pragma once

#include "../world/visual_data_structures.hpp"
#include "camera.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>

namespace app::renderer
{
class World
{
  higanbana::ShaderArgumentsLayout m_meshArgumentsLayout;
  higanbana::GraphicsPipeline m_pipeline;
  higanbana::Renderpass m_renderpass;
public:
  World(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout cameras, higanbana::ShaderArgumentsLayout materials);
  higanbana::ShaderArgumentsLayout& meshArgLayout() {return m_meshArgumentsLayout;}
  void beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth);
  void endRenderpass(higanbana::CommandGraphNode& node);
  higanbana::ShaderArgumentsBinding bindPipeline(higanbana::CommandGraphNode& node);
  void renderMesh(higanbana::CommandGraphNode& node, higanbana::ShaderArgumentsBinding& binding,  higanbana::BufferIBV ibv, int cameraIndex, int prevCamera, int materialIndex, int2 outputSize);
};
}