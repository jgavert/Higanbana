#include "generate_image.hpp"

SHADER_STRUCT(ComputeConstants,
  float resx;
  float resy;
  float time;
  int unused;
);

namespace app::renderer
{
GenerateImage::GenerateImage(higanbana::GpuGroup& device, std::string shaderName, uint3 threadGroups){
  using namespace higanbana;
  higanbana::ShaderArgumentsLayoutDescriptor layoutdesc = ShaderArgumentsLayoutDescriptor()
    .readWrite(ShaderResourceType::Texture2D, "float4", "output");
  m_argLayout = device.createShaderArgumentsLayout(layoutdesc);
  higanbana::PipelineInterfaceDescriptor babyInf2 = PipelineInterfaceDescriptor()
    .constants<ComputeConstants>()
    .shaderArguments(0, m_argLayout);

  m_pipeline = device.createComputePipeline(ComputePipelineDescriptor()
  .setInterface(babyInf2)
  .setShader(shaderName)
  .setThreadGroups(threadGroups));
  time.firstTick();
}
void GenerateImage::generate(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureUAV& uav) {
  using namespace higanbana;
  time.tick();
  auto args = device.createShaderArguments(ShaderArgumentsDescriptor("Generate Texture Descriptors", m_argLayout)
    .bind("output", uav));

  auto binding = node.bind(m_pipeline);
  ComputeConstants consts{};
  consts.time = time.getFTime();
  consts.resx = uav.texture().desc().desc.width; 
  consts.resy = uav.texture().desc().desc.height;
  binding.constants(consts);
  binding.arguments(0, args);

  node.dispatchThreads(binding, uav.texture().desc().desc.size3D());
}
}