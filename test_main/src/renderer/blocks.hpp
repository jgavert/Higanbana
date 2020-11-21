#pragma once

#include "camera.hpp"
#include "../world/visual_data_structures.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>
#include <css/task.hpp>

namespace app::renderer
{
// draws everything with one draw call!... right!!?
class Blocks 
{
  higanbana::ShaderArgumentsLayout triangleLayout;
  higanbana::GraphicsPipeline m_pipeline;
  higanbana::Renderpass m_renderpass;
public:
  SHADER_STRUCT(Constants,
    float4x4 worldTransform;
    ActiveCameraInfo camera;
    float4 position;
    int2 outputSize;
    float scale;
    float time;
    uint cubeMaterial;
  );
  Blocks(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout cameras, higanbana::ShaderArgumentsLayout materials);
  void beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth);
  void endRenderpass(higanbana::CommandGraphNode& node);
  higanbana::ShaderArgumentsBinding bindPipeline(higanbana::CommandGraphNode& node);
  void renderBlocks(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth, higanbana::ShaderArguments cameraArgs, higanbana::ShaderArguments materials, int cameraIndex, int prevCamera, higanbana::vector<ChunkBlockDraw>& instances);
};
}