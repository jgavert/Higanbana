#include "generate_mips.hpp"

SHADER_STRUCT(GenMipsConsts,
  uint2 outputSize;
  uint2 sourceSize;
);

namespace app::renderer
{
GenerateMips::GenerateMips(higanbana::GpuGroup& dev) {
  using namespace higanbana;
  auto inputDataLayout = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Texture2D, "float4", "input")
    .readWrite(ShaderResourceType::Texture2D, "float4", "output");
  m_input = dev.createShaderArgumentsLayout(inputDataLayout);

  PipelineInterfaceDescriptor instancePipeline = PipelineInterfaceDescriptor()
    .constants<GenMipsConsts>()
    .shaderArguments(0, m_input);

  auto pipelineDescriptor = ComputePipelineDescriptor()
    .setInterface(instancePipeline)
    .setShader("generate_mips")
    .setThreadGroups(uint3(8,8,1));

  pipeline = dev.createComputePipeline(pipelineDescriptor);
}
void GenerateMips::generateMipsCS(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::Texture source) {
  using namespace higanbana;
  HIGAN_ASSERT(source.desc().desc.usage == ResourceUsage::GpuRW || source.desc().desc.usage == ResourceUsage::RenderTargetRW, "woo");
  auto mips = source.desc().desc.miplevels-1;
  for (int mip = 0; mip < mips; ++mip) {
    auto srv = dev.createTextureSRV(source, ShaderViewDescriptor().setMostDetailedMip(mip).setMipLevels(1));
    auto uav = dev.createTextureUAV(source, ShaderViewDescriptor().setMostDetailedMip(mip+1).setMipLevels(1));
    generateMipCS(dev, node, srv, uav);
  }
}
void GenerateMips::generateMipCS(higanbana::GpuGroup& dev, higanbana::CommandGraphNode& node, higanbana::TextureSRV sourceMip, higanbana::TextureUAV outputMip) {
  using namespace higanbana;
  auto srcMip = sourceMip.mip();
  auto dstMip = outputMip.mip();
  //HIGAN_LOGi("gen mip %d > %d\n", srcMip, dstMip);
  using namespace higanbana;
  auto bind = node.bind(pipeline);
  auto args = dev.createShaderArguments(ShaderArgumentsDescriptor("genMipsCS", m_input)
    .bind("input", sourceMip)
    .bind("output", outputMip));

  bind.arguments(0, args);
  GenMipsConsts c = {};
  c.sourceSize = sourceMip.size3D().xy();
  c.outputSize = outputMip.size3D().xy();
  bind.constants(c);
  //HIGAN_LOGi("constants %dx%d -> %dx%d\n", c.sourceSize.x, c.sourceSize.y, c.outputSize.x, c.outputSize.y);
  uint3 threads = outputMip.size3D();
  node.dispatchThreads(bind, threads);
}
}