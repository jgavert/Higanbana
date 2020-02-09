#include "mesh_test.hpp"

SHADER_STRUCT(DebugConstants,
  uint meshletCount;
  float3 pos;
);

namespace app
{
MeshTest::MeshTest(higanbana::GpuGroup& device)
{
  using namespace higanbana;
  ShaderArgumentsLayoutDescriptor staticDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly<CameraSettings>(ShaderResourceType::StructuredBuffer, "cameras");
  ShaderArgumentsLayoutDescriptor inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly<WorldMeshlet>(ShaderResourceType::StructuredBuffer, "meshlets")
    .readOnly(ShaderResourceType::Buffer, "uint", "uniqueIndices")
    .readOnly(ShaderResourceType::Buffer, "uint", "packedIndices")
    .readOnly(ShaderResourceType::Buffer, "float3", "vertices")
    .readOnly(ShaderResourceType::Buffer, "float2", "uvs")
    .readOnly(ShaderResourceType::Buffer, "float3", "normals");
  m_staticArgumentsLayout = device.createShaderArgumentsLayout(staticDataLayout);
  m_meshArgumentsLayout = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<DebugConstants>()
    .shaderArguments(0, m_staticArgumentsLayout)
    .shaderArguments(1, m_meshArgumentsLayout);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(instancePipeline)
    .setAmplificationShader("testMeshShaders")
    .setMeshShader("testMeshShaders")
    .setPixelShader("testMeshShaders")
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

void MeshTest::beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureDSV& depth)
{
  node.renderpass(m_renderpass, target, depth);
}

void MeshTest::renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments cameras, higanbana::ShaderArguments meshBuffers, uint meshlets)
{
  auto binding = node.bind(m_pipeline);
  binding.arguments(0, cameras);
  binding.arguments(1, meshBuffers);
  binding.constants(DebugConstants{meshlets, float3(0,0,0)});
  node.dispatchMesh(binding, uint3(1,1,1));
}
}