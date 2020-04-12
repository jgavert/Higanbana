#include "world_renderer.hpp"
#include <higanbana/core/profiling/profiling.hpp>

SHADER_STRUCT(DebugConstants,
  float3 pos;
  int camera;
  int prevCamera;
  int material;
  int2 outputSize;
  float4x4 worldMatrix;
);

namespace app::renderer
{
World::World(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout cameras, higanbana::ShaderArgumentsLayout materials)
{
  using namespace higanbana;
  ShaderArgumentsLayoutDescriptor inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Buffer, "float3", "vertices")
    .readOnly(ShaderResourceType::Buffer, "float2", "uvs")
    .readOnly(ShaderResourceType::Buffer, "float3", "normals")
    .readOnly(ShaderResourceType::Buffer, "float3", "tangents");
  m_meshArgumentsLayout = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<DebugConstants>()
    .shaderArguments(0, cameras)
    .shaderArguments(1, m_meshArgumentsLayout)
    .shaderArguments(2, materials);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(instancePipeline)
    .setVertexShader("world")
    .setPixelShader("world")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setRTVFormat(0, FormatType::Unorm16RGBA)
    .setRTVFormat(1, FormatType::Float16RGBA)
    .setDSVFormat(FormatType::Depth32)
    .setRenderTargetCount(2)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(true)
      .setDepthFunc(ComparisonFunc::Greater));

  m_pipeline = device.createGraphicsPipeline(pipelineDescriptor);
  m_renderpass = device.createRenderpass();
}

void World::beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  node.renderpass(m_renderpass, target, motionVecs, depth);
}
void World::endRenderpass(higanbana::CommandGraphNode& node)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  node.endRenderpass();
}

higanbana::ShaderArgumentsBinding World::bindPipeline(higanbana::CommandGraphNode& node) {
  return node.bind(m_pipeline);
}

void World::renderMesh(higanbana::CommandGraphNode& node, higanbana::ShaderArgumentsBinding& binding, higanbana::BufferIBV ibv, int cameraIndex, int prevCamera, int materialIndex, int2 outputSize, float4x4 worldTransform)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  binding.constants(DebugConstants{float3(0,0,0), cameraIndex, prevCamera, materialIndex, outputSize, worldTransform});
  node.drawIndexed(binding, ibv, ibv.viewDesc().m_elementCount);
}
}