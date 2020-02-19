#include "particles.hpp"

SHADER_STRUCT(SimulConstants,
  float4 originPoint;
  float frameTimeDiff;
  uint maxParticles;
);

namespace app::renderer
{
Particles::Particles(higanbana::GpuGroup& device) {
  using namespace higanbana;

  m_particles.resize(device, ResourceDescriptor()
    .setStructured<Particles::Particle>()
    .setCount(100)
    .setUsage(ResourceUsage::GpuRW)
    .setDimension(FormatDimension::Buffer));

  m_simulationLayout = device.createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor()
    .readOnly<Particles::Particle>(ShaderResourceType::StructuredBuffer, "previous")
    .readWrite<Particles::Particle>(ShaderResourceType::StructuredBuffer, "current"));
  m_simulationPipeline = device.createComputePipeline(ComputePipelineDescriptor()
    .setInterface(PipelineInterfaceDescriptor()
      .constants<SimulConstants>()
      .shaderArguments(0, m_simulationLayout))
    .setShader("particles")
    .setThreadGroups(uint3(256, 1, 1)));
}
void Particles::simulate(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node) {
  using namespace higanbana;

  m_particles.next();

  SimulConstants consts{};
  consts.originPoint = float4(0,0,0,1);
  consts.frameTimeDiff = 16.66667f;
  consts.maxParticles = m_particles.desc().desc.width;
  
  ShaderArgumentsDescriptor desc = ShaderArgumentsDescriptor("simulation", m_simulationLayout)
      .bind("previous", m_particles.previousSrv())
      .bind("current", m_particles.currentUav());
  auto set = dev.createShaderArguments(desc);
  auto binding = node.bind(m_simulationPipeline);
  binding.arguments(0, set);
  binding.constants(consts);

  node.dispatchThreads(binding, uint3(m_particles.desc().desc.size3D()));
}
}