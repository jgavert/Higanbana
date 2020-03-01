#include "mesh_test.hpp"

SHADER_STRUCT(DebugConstants,
  uint meshletCount;
  int camera;
  float2 unused;
);
namespace app::renderer
{
MeshTest::MeshTest(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout camerasLayout, higanbana::ShaderArgumentsLayout materials)
{
  using namespace higanbana;
  ShaderArgumentsLayoutDescriptor inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly<WorldMeshlet>(ShaderResourceType::StructuredBuffer, "meshlets")
    .readOnly(ShaderResourceType::Buffer, "uint", "uniqueIndices")
    .readOnly(ShaderResourceType::Buffer, "uint", "packedIndices")
    .readOnly(ShaderResourceType::Buffer, "float3", "vertices")
    .readOnly(ShaderResourceType::Buffer, "float2", "uvs")
    .readOnly(ShaderResourceType::Buffer, "float3", "normals");
  m_meshArgumentsLayout = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<DebugConstants>()
    .shaderArguments(0, camerasLayout)
    .shaderArguments(1, m_meshArgumentsLayout)
    .shaderArguments(2, materials);

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

void MeshTest::renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments cameras, higanbana::ShaderArguments meshBuffers, higanbana::ShaderArguments materials, uint meshlets, int cameraIndex)
{
  auto binding = node.bind(m_pipeline);
  binding.arguments(0, cameras);
  binding.arguments(1, meshBuffers);
  binding.arguments(2, materials);
  binding.constants(DebugConstants{meshlets, cameraIndex});
  node.dispatchMesh(binding, uint3(1,1,1));
}
}