#include "tsaa.hpp"
SHADER_STRUCT(TSAAConstants,
  uint2 outputSize;
  uint2 sourceSize;
  float2 jitter;
);

namespace app::renderer
{
ShittyTSAA::ShittyTSAA(higanbana::GpuGroup& device) {
  using namespace higanbana;
  auto inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Texture2D, "float4", "history")
    .readOnly(ShaderResourceType::Texture2D, "float4", "source")
    .readOnly(ShaderResourceType::Texture2D, "float4", "motion")
    .readWrite(ShaderResourceType::Texture2D, "float4", "result")
    .readWrite(ShaderResourceType::Texture2D, "float4", "debug");
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
float4x4 ShittyTSAA::jitterProjection(int frame, int2 outputSize, float4x4 projection){
  using namespace higanbana::math;
  // Compute jittered matrices

  // Let's assume that we are using 8x
  constexpr static const int SAMPLE_COUNT = 8;

  unsigned SubsampleIdx = frame % SAMPLE_COUNT;
    
  const float2 TexSize(div(1.0f, float2(outputSize))); // Texel size
  const float2 SubsampleSize = mul(TexSize, 2.0f); // That is the size of the subsample in NDC
  auto currentJitter = SampleLocations8[SubsampleIdx];
  const float2 S = div(float2(currentJitter), 8.0f); // In [-1, 1]
    
  float2 Subsample = mul(S, SubsampleSize); // In [-SubsampleSize, SubsampleSize] range
  Subsample = mul(Subsample, 0.5f); // In [-SubsampleSize / 2, SubsampleSize / 2] range

  float4x4 jitterMatrix = translation(float3(Subsample, 0.f));
  return mul(projection, jitterMatrix);
}

void ShittyTSAA::resolve(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureUAV output, TSAAArguments args) {
  using namespace higanbana;
  auto argument = device.createShaderArguments(ShaderArgumentsDescriptor("tsaa resolve", m_args)
    .bind("history", args.history)
    .bind("source", args.source)
    .bind("motion", args.motionVectors)
    .bind("result", output)
    .bind("debug", args.debug));
  auto binding = node.bind(m_pipeline);
  binding.arguments(0, argument);
  TSAAConstants consts{};
  consts.outputSize = output.texture().desc().desc.size3D().xy();
  consts.sourceSize = args.source.texture().desc().desc.size3D().xy();
  consts.jitter = div(float2(args.jitterOffset), 8.0f);
  binding.constants(consts);
  node.dispatchThreads(binding, output.texture().desc().desc.size3D());
}
}