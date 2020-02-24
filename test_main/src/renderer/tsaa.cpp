#include "tsaa.hpp"
SHADER_STRUCT(TSAAConstants,
  uint2 outputSize;
);

namespace app::renderer
{
ShittyTSAA::ShittyTSAA(higanbana::GpuGroup& device) {
  using namespace higanbana;
  auto inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Texture2D, "float4", "history")
    .readOnly(ShaderResourceType::Texture2D, "float4", "source")
    .readWrite(ShaderResourceType::Texture2D, "float4", "result");
  m_args = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<TSAAConstants>()
    .shaderArguments(0, m_args);

  auto pipelineDescriptor = ComputePipelineDescriptor()
    .setInterface(instancePipeline)
    .setShader("tsaa")
    .setThreadGroups(uint3(8,8,1));

  m_pipeline = device.createComputePipeline(pipelineDescriptor);
}

void ShittyTSAA::resolve(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureUAV output, TSAAArguments args) {
  using namespace higanbana;
  auto argument = device.createShaderArguments(ShaderArgumentsDescriptor("Opaque Arguments", m_args)
    .bind("history", args.history)
    .bind("source", args.source)
    .bind("result", output));
  auto binding = node.bind(m_pipeline);
  binding.arguments(0, argument);
  TSAAConstants consts{};
  consts.outputSize = output.texture().desc().desc.size3D().xy();
  binding.constants(consts);
  node.dispatchThreads(binding, output.texture().desc().desc.size3D());
}
}