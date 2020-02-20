#include "world_renderer.hpp"

SHADER_STRUCT(DebugConstants,
  float3 pos;
);

namespace app::renderer
{
World::World(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout cameras)
{
  using namespace higanbana;
  ShaderArgumentsLayoutDescriptor inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Buffer, "float3", "vertices")
    .readOnly(ShaderResourceType::Buffer, "float2", "uvs")
    .readOnly(ShaderResourceType::Buffer, "float3", "normals");
  m_meshArgumentsLayout = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<DebugConstants>()
    .shaderArguments(0, cameras)
    .shaderArguments(1, m_meshArgumentsLayout);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(instancePipeline)
    .setVertexShader("world")
    .setPixelShader("world")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setRasterizer(RasterizerDescriptor().setFrontCounterClockwise(false))
    .setRTVFormat(0, FormatType::Unorm8BGRA)
    .setDSVFormat(FormatType::Depth32)
    .setRenderTargetCount(1)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(true)
      .setDepthFunc(ComparisonFunc::Greater));

  m_pipeline = device.createGraphicsPipeline(pipelineDescriptor);
  m_renderpass = device.createRenderpass();
}

void World::beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureDSV& depth)
{
  node.renderpass(m_renderpass, target, depth);
}
void World::endRenderpass(higanbana::CommandGraphNode& node)
{
  node.endRenderpass();
}

void World::renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments cameras, higanbana::ShaderArguments meshBuffers)
{
  auto binding = node.bind(m_pipeline);
  binding.arguments(0, cameras);
  binding.arguments(1, meshBuffers);
  binding.constants(DebugConstants{float3(0,0,0)});
  node.drawIndexed(binding, ibv, ibv.desc().desc.width);
}
}