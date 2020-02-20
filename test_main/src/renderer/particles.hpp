#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/pingpongBuffer.hpp>

namespace app::renderer
{
// test system so this will break conventions on purpose, owns its own buffers
class Particles
{
  higanbana::ShaderArgumentsLayout m_simulationLayout;
  higanbana::ComputePipeline m_simulationPipeline;
  higanbana::PingPongBuffer m_particles;

  higanbana::ShaderArgumentsLayout m_drawLayout;
  higanbana::GraphicsPipeline m_drawPipeline;
  higanbana::Renderpass m_renderpass;
public:
  SHADER_STRUCT(Particle,
    float4 pos;
    float4 velocity;
  );
  Particles(higanbana::GpuGroup& dev, higanbana::ShaderArgumentsLayout cameras);
  void simulate(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node);
  void render(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::TextureRTV backbuffer, higanbana::TextureDSV depth, higanbana::ShaderArguments cameras);
};
}