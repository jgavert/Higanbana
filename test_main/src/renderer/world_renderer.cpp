#include "world_renderer.hpp"

SHADER_STRUCT(DebugConstants,
  float3 pos;
  int camera;
  int prevCamera;
  int material;
  int2 outputSize;
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
  node.renderpass(m_renderpass, target, motionVecs, depth);
  outputSize = target.texture().desc().desc.size3D().xy();
}
void World::endRenderpass(higanbana::CommandGraphNode& node)
{
  node.endRenderpass();
}

void World::renderMesh(higanbana::CommandGraphNode& node, higanbana::BufferIBV ibv, higanbana::ShaderArguments cameras, higanbana::ShaderArguments meshBuffers, higanbana::ShaderArguments materials, int cameraIndex, int prevCamera, int materialIndex)
{
  auto binding = node.bind(m_pipeline);
  binding.arguments(0, cameras);
  binding.arguments(1, meshBuffers);
  binding.arguments(2, materials);
  binding.constants(DebugConstants{float3(0,0,0), cameraIndex, prevCamera, materialIndex, outputSize});
  node.drawIndexed(binding, ibv, ibv.viewDesc().m_elementCount);
}
}