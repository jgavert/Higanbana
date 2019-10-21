#include "world_renderer.hpp"

SHADER_STRUCT(DebugConstants,
  float3 pos;
);

namespace app
{
WorldRenderer::WorldRenderer(higanbana::GpuGroup& device)
{
  using namespace higanbana;
  ShaderArgumentsLayoutDescriptor staticDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly<CameraSettings>(ShaderResourceType::StructuredBuffer, "cameras");
  ShaderArgumentsLayoutDescriptor inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Buffer, "float3", "vertices");
  m_staticArgumentsLayout = device.createShaderArgumentsLayout(staticDataLayout);
  m_meshArgumentsLayout = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<DebugConstants>()
    .shaderArguments(0, m_staticArgumentsLayout)
    .shaderArguments(1, m_meshArgumentsLayout);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(instancePipeline)
    .setVertexShader("world")
    .setPixelShader("world")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setRasterizer(RasterizerDescriptor())
    .setRTVFormat(0, FormatType::Unorm8BGRA)
    .setDSVFormat(FormatType::Depth32)
    .setRenderTargetCount(1)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(true)
      .setDepthFunc(ComparisonFunc::Greater));

  m_pipeline = device.createGraphicsPipeline(pipelineDescriptor);
  m_renderpass = device.createRenderpass();
}

void WorldRenderer::beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureDSV& depth)
{
  node.renderpass(m_renderpass, target, depth);
}
void WorldRenderer::endRenderpass(higanbana::CommandGraphNode& node)
{
  node.endRenderpass();
}

void WorldRenderer::renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments cameras, higanbana::ShaderArguments meshBuffers)
{
  auto binding = node.bind(m_pipeline);
  binding.arguments(0, cameras);
  binding.arguments(1, meshBuffers);
  binding.constants(DebugConstants{float3(0,0,0)});
  node.drawIndexed(binding, ibv, ibv.desc().desc.width);
}
}