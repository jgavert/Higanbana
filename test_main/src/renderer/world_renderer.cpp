#include "world_renderer.hpp"

SHADER_STRUCT(DebugConstants,
  float3 pos;
);

namespace app
{
WorldRenderer::WorldRenderer(higanbana::GpuGroup& device)
{
  using namespace higanbana;
  ShaderArgumentsLayoutDescriptor argsLayoutDesc = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Buffer, "float3", "vertices");
  m_pipelineLayout = device.createShaderArgumentsLayout(argsLayoutDesc);

  higanbana::PipelineInterfaceDescriptor imguiInterface = PipelineInterfaceDescriptor()
    .constants<DebugConstants>()
    .shaderArguments(0, m_pipelineLayout);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(imguiInterface)
    .setVertexShader("world")
    .setPixelShader("world")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false))

    .setRasterizer(RasterizerDescriptor()
      .setCullMode(CullMode::None)
      .setFillMode(FillMode::Solid))
    .setRenderTargetCount(1)
    .setBlend(BlendDescriptor()
      .setRenderTarget(0, RTBlendDescriptor()
        .setBlendEnable(true)
        .setSrcBlend(Blend::SrcAlpha)
        .setDestBlend(Blend::InvSrcAlpha)
        .setBlendOp(BlendOp::Add)
        .setSrcBlendAlpha(Blend::SrcAlpha)
        .setDestBlendAlpha(Blend::InvSrcAlpha)
        .setBlendOpAlpha(BlendOp::Add)))
    ;

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

void WorldRenderer::renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments meshBuffers)
{
  auto& binding = node.bind(m_pipeline);
  binding.arguments(0, meshBuffers);
  binding.constants(DebugConstants{float3(0,0,0)});
  node.drawIndexed(binding, ibv, ibv.desc().desc.width);
}
}