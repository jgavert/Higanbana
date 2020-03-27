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
  int2 outputSize;
public:
  World(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout cameras, higanbana::ShaderArgumentsLayout materials);
  higanbana::ShaderArgumentsLayout& meshArgLayout() {return m_meshArgumentsLayout;}
  void beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth);
  void endRenderpass(higanbana::CommandGraphNode& node);
  void renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments cameras, higanbana::ShaderArguments meshBuffers, higanbana::ShaderArguments materials, int cameraIndex, int prevCamera, int materialIndex);
};
}