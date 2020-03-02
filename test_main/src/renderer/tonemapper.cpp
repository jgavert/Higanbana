#include "tonemapper.hpp"
SHADER_STRUCT(TonemapperConstants,
  uint2 outputSize;
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
    .setRTVFormat(0, FormatType::Unorm8BGRA)
    .setRenderTargetCount(1)
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false));

  pipelineBGRA = device.createGraphicsPipeline(pipelineDescriptor);
  pipelineRGBA = device.createGraphicsPipeline(pipelineDescriptor.setRTVFormat(0, FormatType::Unorm8RGBA));
  m_renderpassBGRA = device.createRenderpass();
  m_renderpassRGBA = device.createRenderpass();
}

void Tonemapper::tonemap(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV output, TonemapperArguments args) {
  using namespace higanbana;

  if (output.texture().desc().desc.format == FormatType::Unorm8BGRA)
    node.renderpass(m_renderpassBGRA, output);
  else
    node.renderpass(m_renderpassRGBA, output);

  auto argument = device.createShaderArguments(ShaderArgumentsDescriptor("tonemapper source", m_args)
    .bind("source", args.source));
  ShaderArgumentsBinding binding;
  if (output.texture().desc().desc.format == FormatType::Unorm8BGRA)
    binding = node.bind(pipelineBGRA);
  else
    binding = node.bind(pipelineRGBA);
  
  binding.arguments(0, argument);
  TonemapperConstants consts{};
  consts.outputSize = output.texture().desc().desc.size3D().xy();
  binding.constants(consts);

  node.draw(binding, 3);

  node.endRenderpass();
}
}