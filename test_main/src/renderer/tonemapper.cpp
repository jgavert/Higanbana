#include "tonemapper.hpp"
SHADER_STRUCT(TonemapperConstants,
  uint2 outputSize;
  uint eotf;
  uint tsaaActive;
);

namespace app::renderer
{
Tonemapper::Tonemapper(higanbana::GpuGroup& device) {
  using namespace higanbana;
  auto inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Texture2D, "float4", "source");
  m_args = device.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<TonemapperConstants>()
    .shaderArguments(0, m_args);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(instancePipeline)
    .setVertexShader("tonemapper")
    .setPixelShader("tonemapper")
    .setRenderTargetCount(1)
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false));

  for (int i = 0; i < static_cast<int>(FormatType::Count); i++) {
    auto formatInfo = formatSizeInfo(static_cast<FormatType>(i));
    pipelines(formatInfo.fm) = device.createGraphicsPipeline(pipelineDescriptor.setRTVFormat(0, formatInfo.fm));
    renderpasses(formatInfo.fm) = device.createRenderpass();
  }
}

void Tonemapper::tonemap(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV output, TonemapperArguments args) {
  using namespace higanbana;

  auto& renderpass = renderpasses(output.texture().desc().desc.format);
  HIGAN_ASSERT(renderpass, "should be valid renderpass");
  node.renderpass(renderpass, output);

  auto argument = device.createShaderArguments(ShaderArgumentsDescriptor("tonemapper source", m_args)
    .bind("source", args.source));
  auto& pipeline = pipelines(output.texture().desc().desc.format);
  HIGAN_ASSERT(pipeline, "pipeline should be valid");
  auto binding = node.bind(pipeline);
  
  binding.arguments(0, argument);
  TonemapperConstants consts{};
  consts.outputSize = output.texture().desc().desc.size3D().xy();
  consts.eotf = (output.texture().desc().desc.format == FormatType::Unorm10RGB2A) ? 1 : 0;
  consts.tsaaActive = (args.tsaa) ? 1 : 0;
  binding.constants(consts);

  node.draw(binding, 3);

  node.endRenderpass();
}
}