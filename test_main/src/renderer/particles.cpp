#include "particles.hpp"

SHADER_STRUCT(SimulConstants,
  float4 originPoint;
  float frameTimeDiff;
  uint maxParticles;
);

SHADER_STRUCT(DrawConstants,

  float4 color;
);

namespace app::renderer
{
Particles::Particles(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout cameras) {
  using namespace higanbana;

  m_particles.resize(device, ResourceDescriptor()
    .setStructured<Particles::Particle>()
    .setElementsCount(2000)
    .setUsage(ResourceUsage::GpuRW)
    .setDimension(FormatDimension::Buffer));

  m_drawLayout = device.createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor()
    .readOnly<Particles::Particle>(ShaderResourceType::StructuredBuffer, "particles"));
  m_simulationLayout = device.createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor()
    .readOnly<Particles::Particle>(ShaderResourceType::StructuredBuffer, "previous")
    .readWrite<Particles::Particle>(ShaderResourceType::StructuredBuffer, "current"));
  m_simulationPipeline = device.createComputePipeline(ComputePipelineDescriptor()
    .setInterface(PipelineInterfaceDescriptor()
      .constants<SimulConstants>()
      .shaderArguments(0, m_simulationLayout))
    .setShader("particles")
    .setThreadGroups(uint3(256, 1, 1)));

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<DrawConstants>()
    .shaderArguments(0, m_drawLayout)
    .shaderArguments(1, cameras);
  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(instancePipeline)
    .setVertexShader("/shaders/draw_particles")
    .setPixelShader("/shaders/draw_particles")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setRasterizer(RasterizerDescriptor().setFrontCounterClockwise(false))
    .setRTVFormat(0, FormatType::Unorm8BGRA)
    .setDSVFormat(FormatType::Depth32)
    .setRenderTargetCount(1)
    .setBlend(BlendDescriptor()
      .setIndependentBlendEnable(true)
      .setRenderTarget(0, RTBlendDescriptor()
        .setBlendEnable(true)
        .setBlendOpAlpha(BlendOp::Add)
        .setSrcBlend(Blend::SrcAlpha)
        .setSrcBlendAlpha(Blend::One)
        .setDestBlend(Blend::InvSrcAlpha)
        .setDestBlendAlpha(Blend::One)))
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(true)
      .setDepthWriteMask(DepthWriteMask::Zero)
      .setDepthFunc(ComparisonFunc::Greater));
  m_drawPipeline = device.createGraphicsPipeline(pipelineDescriptor);
  m_renderpass = device.createRenderpass();
}
void Particles::simulate(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, float deltaTime) {
  using namespace higanbana;

  m_particles.next();

  SimulConstants consts{};
  consts.originPoint = float4(0,0,0,1);
  consts.frameTimeDiff = deltaTime;
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
void Particles::render(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::TextureRTV backbuffer, higanbana::TextureDSV depth, higanbana::ShaderArguments cameras) {
  using namespace higanbana;
  depth.setOp(LoadOp::Load);
  backbuffer.setOp(LoadOp::Load);
  node.renderpass(m_renderpass, backbuffer, depth);
  ShaderArgumentsDescriptor desc = ShaderArgumentsDescriptor("draw particles", m_drawLayout)
      .bind("particles", m_particles.currentSrv());
  auto set = dev.createShaderArguments(desc);
  auto binding = node.bind(m_drawPipeline);
  binding.arguments(0, set);
  binding.arguments(1, cameras);
  binding.constants(DrawConstants{float4(1,0,0,1)});
  node.draw(binding, 6, m_particles.desc().desc.width);
  node.endRenderpass();
}
}