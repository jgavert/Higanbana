#include "depth_pyramid.hpp"

SHADER_STRUCT(DepthPyramidConsts,
  uint2 outputSize;
  uint2 sourceSize;
);

namespace app::renderer
{
DepthPyramid::DepthPyramid(higanbana::GpuGroup& device) {
  using namespace higanbana;
  auto inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Texture2D, "float4", "depthbuffer")
    .readWrite(ShaderResourceType::ByteAddressBuffer, "uint", "counters")
    .readWrite(ShaderResourceType::Texture2D, "float4", "pyramid");
  m_input = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<DepthPyramidConsts>()
    .shaderArguments(0, m_input);

  auto pipelineDescriptor = ComputePipelineDescriptor()
    .setInterface(instancePipeline)
    .setShader("depthpyramid")
    .setThreadGroups(uint3(8,8,1));

  pipeline = device.createComputePipeline(pipelineDescriptor);

  m_counters = device.createBuffer(ResourceDescriptor()
    .setCount(64*64)
    .setUsage(ResourceUsage::GpuRW)
    .setFormat(FormatType::Raw32));
  m_countersUAV = device.createBufferUAV(m_counters);
}

higanbana::ResourceDescriptor DepthPyramid::getMinimumDepthPyramidDescriptor(higanbana::TextureSRV depthBuffer) {
  auto size = depthBuffer.desc().desc.size3D();
  //size.x += static_cast<int>((float(size.x) * 0.5f) + 0.5f);
  higanbana::ResourceDescriptor desc = higanbana::ResourceDescriptor()
    .setSize(size)
    .setMiplevels(12)
    .setUsage(higanbana::ResourceUsage::GpuRW)
    .setFormat(higanbana::FormatType::Float32);
  return desc;
}

void DepthPyramid::downsample(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureSRV depthBuffer, higanbana::TextureUAV pyramidOutput){
  using namespace higanbana;
  auto argument = device.createShaderArguments(ShaderArgumentsDescriptor("downsample", m_input)
    .bind("depthbuffer", depthBuffer)
    .bind("counters", m_countersUAV)
    .bind("pyramid", pyramidOutput));
  auto binding = node.bind(pipeline);
  binding.arguments(0, argument);
  DepthPyramidConsts consts{};
  consts.outputSize = depthBuffer.texture().desc().desc.size3D().xy();
  consts.sourceSize = pyramidOutput.texture().desc().desc.size3D().xy();
  binding.constants(consts);
  node.dispatchThreads(binding, depthBuffer.texture().desc().desc.size3D());
}
}
