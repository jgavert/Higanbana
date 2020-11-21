#include "blocks.hpp"
#include <higanbana/core/profiling/profiling.hpp>
#include <higanbana/core/global_debug.hpp>

namespace app::renderer
{
Blocks::Blocks(higanbana::GpuGroup& device, higanbana::ShaderArgumentsLayout cameras, higanbana::ShaderArgumentsLayout materials) {
  using namespace higanbana;
  higanbana::ShaderArgumentsLayoutDescriptor triangleLayoutDesc = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::ByteAddressBuffer, "vertexInput");
  triangleLayout = device.createShaderArgumentsLayout(triangleLayoutDesc);
  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .structDecl<ActiveCameraInfo>()
    .constants<Blocks::Constants>()
    .shaderArguments(0, triangleLayout)
    .shaderArguments(1, cameras)
    .shaderArguments(2, materials);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(instancePipeline)
    .setVertexShader("blocksSimple")
    .setPixelShader("blocksSimple")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setRTVFormat(0, FormatType::Float16RGBA)
    .setRTVFormat(1, FormatType::Float16RGBA)
    .setDSVFormat(FormatType::Depth32)
    //.setRasterizer(RasterizerDescriptor().setCullMode(CullMode::None))
    .setRenderTargetCount(2)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(true)
      .setDepthFunc(ComparisonFunc::Greater));

  m_pipeline = device.createGraphicsPipeline(pipelineDescriptor);
  m_renderpass = device.createRenderpass();

  // cube index buffer;

}
void Blocks::beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV& target, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth)
{
  using namespace higanbana;
  HIGAN_CPU_FUNCTION_SCOPE();
  HIGAN_ASSERT(target.desc().desc.format == FormatType::Float16RGBA, "");
  HIGAN_ASSERT(motionVecs.desc().desc.format == FormatType::Float16RGBA, "");
  HIGAN_ASSERT(depth.desc().desc.format == FormatType::Depth32, "");
  node.renderpass(m_renderpass, target, motionVecs, depth);
}
void Blocks::endRenderpass(higanbana::CommandGraphNode& node)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  node.endRenderpass();
}

higanbana::ShaderArgumentsBinding Blocks::bindPipeline(higanbana::CommandGraphNode& node) {
  return node.bind(m_pipeline);
}

void Blocks::renderBlocks(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth, higanbana::ShaderArguments cameraArgs, higanbana::ShaderArguments materials, int cameraIndex, int prevCamera, higanbana::vector<ChunkBlockDraw>& instances) {
  using namespace higanbana;
  HIGAN_CPU_FUNCTION_SCOPE();
  backbuffer.setOp(LoadOp::Load);
  depth.clearOp({});
  beginRenderpass(node, backbuffer, motionVecs, depth);
  vector<uint> vertexData = {
      0,  1,  1, //0.f, 0.66f, // 0
      0,  0,  1, //0.25f, 0.66f, // 1
      0,  1,  0, //0.f, 0.33f, // 2
      0,  0,  0, //0.25f, 0.33f, // 3
      1,  0,  1, //0.5f, 0.66f, // 4
      1,  0,  0, //0.5f, 0.33f, // 5
      1,  1,  1, //0.75f, 0.66f, // 6
      1,  1,  0, //0.75f, 0.33f, // 7
      0,  1,  1, //1.f, 0.66f, // 8
      0,  1,  0, //1.f, 0.33f, // 9
      0,  1,  1, //0.25f, 1.f, // 10
      1,  1,  1, //.0.5f, 1.f, // 11
      0,  1,  0, //0.25f, 0.f, // 12
      1,  1,  0, //0.5f, 0.f, // 13
  };

  vector<uint> vertexData2;

  for (int i = 0; i < vertexData.size(); i+=3) {
    uint compressed = (vertexData[i] << 2) | (vertexData[i+1] << 1) | vertexData[i+2];
    vertexData2.push_back(compressed); 
  }

  auto vert = dev.dynamicBuffer<uint>(vertexData2, FormatType::Raw32);
  auto args = dev.createShaderArguments(ShaderArgumentsDescriptor("vertexdata", triangleLayout)
    .bind("vertexInput", vert));
    
  vector<uint> indexData = {
    0, 2, 1,
    2, 3, 1,
    1, 3, 4,
    3, 5, 4,
    4, 5, 6,
    5, 7, 6,
    6, 7, 8,
    7, 9, 8,
    10, 1, 11,
    1, 4, 11,
    3, 12, 5,
    12, 13, 5,
  };
  for (int i = 0; i < indexData.size(); ++i) {
    auto face = i / 6;
    indexData[i] = indexData[i] | (face << 4);
  }
  auto ind = dev.dynamicBuffer<uint>(indexData, FormatType::Uint32);

  auto binding = bindPipeline(node);
  binding.arguments(0, args);
  binding.arguments(1, cameraArgs);
  binding.arguments(2, materials);
  Constants consts{};
  consts.camera.current = cameraIndex;
  consts.camera.previous = prevCamera;
  consts.time = 0;
  consts.scale = 0;
  consts.outputSize = backbuffer.desc().desc.size3D().xy();

  for (auto&& instance : instances)
  {
    consts.position = float4(instance.position, 1.f);
    consts.cubeMaterial = instance.materialIndex;
    binding.constants(consts);
    node.drawIndexed(binding, ind, 12*3);
  }
  endRenderpass(node);
}
}