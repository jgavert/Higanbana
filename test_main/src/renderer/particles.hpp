#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/pingpongBuffer.hpp>

namespace app::renderer
{
// test system so this will break conventions on purpose, owns its own buffer
class Particles
{
  higanbana::ShaderArgumentsLayout m_simulationLayout;
  higanbana::ComputePipeline m_simulationPipeline;
  higanbana::PingPongBuffer m_particles;
public:
  SHADER_STRUCT(Particle,
    float4 pos;
    float4 velocity;
  );
  Particles(higanbana::GpuGroup& dev);
  void simulate(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node);
};
}