#include "higanbana/graphics/common/pipeline_descriptor.hpp"
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
RaytracingPipelineDescriptor::RaytracingPipelineDescriptor()
{
}

RaytracingPipelineDescriptor& RaytracingPipelineDescriptor::setRayHitGroup(int index, RayHitGroupDescriptor hitgroup) {
  HIGAN_ASSERT(index >= 0 && index < 64, "sanity check for index");
  if (hitGroups.size() < index) hitGroups.resize(index+1);
  hitGroups[index] = hitgroup;
  return *this;
}

RaytracingPipelineDescriptor& RaytracingPipelineDescriptor::setRayGen(std::string path) {
  rayGenerationShader = path;
  return *this;
}
RaytracingPipelineDescriptor& RaytracingPipelineDescriptor::setMissShader(std::string path) {
  missShader = path;
  return *this;
}

RaytracingPipelineDescriptor& RaytracingPipelineDescriptor::setInterface(PipelineInterfaceDescriptor val) {
  layout = val;
  return *this;
}

ComputePipelineDescriptor::ComputePipelineDescriptor()
{
}

const char* ComputePipelineDescriptor::shader() const
{
  return shaderSourcePath.c_str();
}

ComputePipelineDescriptor& ComputePipelineDescriptor::setShader(std::string path)
{
  shaderSourcePath = path;
  return *this;
}

ComputePipelineDescriptor& ComputePipelineDescriptor::setThreadGroups(uint3 val)
{
  shaderGroups = val;
  return *this;
}

ComputePipelineDescriptor& ComputePipelineDescriptor::setInterface(PipelineInterfaceDescriptor val)
{
  layout = val;
  return *this;
}

RTBlendDescriptor::RTBlendDescriptor()
{}

RTBlendDescriptor& RTBlendDescriptor::setBlendEnable(bool value)
{
  desc.blendEnable = value;
  return *this;
}
/*
RTBlendDescriptor& RTBlendDescriptor::setLogicOpEnable(bool value)
{
  desc.logicOpEnable = value;
  return *this;
}*/
RTBlendDescriptor& RTBlendDescriptor::setSrcBlend(Blend value)
{
  desc.srcBlend = value;
  return *this;
}
RTBlendDescriptor& RTBlendDescriptor::setDestBlend(Blend value)
{
  desc.destBlend = value;
  return *this;
}
RTBlendDescriptor& RTBlendDescriptor::setBlendOp(BlendOp value)
{
  desc.blendOp = value;
  return *this;
}
RTBlendDescriptor& RTBlendDescriptor::setSrcBlendAlpha(Blend value)
{
  desc.srcBlendAlpha = value;
  return *this;
}
RTBlendDescriptor& RTBlendDescriptor::setDestBlendAlpha(Blend value)
{
  desc.destBlendAlpha = value;
  return *this;
}
RTBlendDescriptor& RTBlendDescriptor::setBlendOpAlpha(BlendOp value)
{
  desc.blendOpAlpha = value;
  return *this;
}
/*
RTBlendDescriptor& RTBlendDescriptor::setLogicOp(LogicOp value)
{
  desc.logicOp = value;
  return *this;
}*/
RTBlendDescriptor& RTBlendDescriptor::setColorWriteEnable(ColorWriteEnable value)
{
  desc.colorWriteEnable = value;
  return *this;
}

RasterizerDescriptor::RasterizerDescriptor()
{}
RasterizerDescriptor& RasterizerDescriptor::setFillMode(FillMode value)
{
  desc.fill = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setCullMode(CullMode value)
{
  desc.cull = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setFrontCounterClockwise(bool value)
{
  desc.frontCounterClockwise = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setDepthBias(int value)
{
  desc.depthBias = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setDepthBiasClamp(float value)
{
  desc.depthBiasClamp = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setSlopeScaledDepthBias(float value)
{
  desc.slopeScaledDepthBias = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setDepthClipEnable(bool value)
{
  desc.depthClipEnable = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setMultisampleEnable(bool value)
{
  desc.multisampleEnable = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setAntialiasedLineEnable(bool value)
{
  desc.antialiasedLineEnable = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setForcedSampleCount(unsigned int value)
{
  desc.forcedSampleCount = value;
  return *this;
}
RasterizerDescriptor& RasterizerDescriptor::setConservativeRaster(ConservativeRasterization value)
{
  desc.conservativeRaster = value;
  return *this;
}
DepthStencilDescriptor::DepthStencilDescriptor()
{
}

DepthStencilDescriptor& DepthStencilDescriptor::setDepthEnable(bool value)
{
  desc.depthEnable = value;
  return *this;
}
DepthStencilDescriptor& DepthStencilDescriptor::setDepthWriteMask(DepthWriteMask value)
{
  desc.depthWriteMask = value;
  return *this;
}
DepthStencilDescriptor& DepthStencilDescriptor::setDepthFunc(ComparisonFunc value)
{
  desc.depthFunc = value;
  return *this;
}
DepthStencilDescriptor& DepthStencilDescriptor::setStencilEnable(bool value)
{
  desc.stencilEnable = value;
  return *this;
}
DepthStencilDescriptor& DepthStencilDescriptor::setStencilReadMask(uint8_t value)
{
  desc.stencilReadMask = value;
  return *this;
}
DepthStencilDescriptor& DepthStencilDescriptor::setStencilWriteMask(uint8_t value)
{
  desc.stencilWriteMask = value;
  return *this;
}
DepthStencilDescriptor& DepthStencilDescriptor::setFrontFace(DepthStencilOpDesc value)
{
  desc.frontFace = value;
  return *this;
}
DepthStencilDescriptor& DepthStencilDescriptor::setBackFace(DepthStencilOpDesc value)
{
  desc.backFace = value;
  return *this;
}

BlendDescriptor::BlendDescriptor()
{
  desc.renderTarget.at(0) = RTBlendDescriptor();
}
BlendDescriptor& BlendDescriptor::setAlphaToCoverageEnable(bool value)
{
  desc.alphaToCoverageEnable = value;
  return *this;
}
BlendDescriptor& BlendDescriptor::setIndependentBlendEnable(bool value)
{
  desc.independentBlendEnable = value;
  return *this;
}
BlendDescriptor& BlendDescriptor::setLogicOp(LogicOp value)
{
  desc.logicOp = value;
  return *this;
}
BlendDescriptor& BlendDescriptor::setLogicOpEnable(bool value)
{
  desc.logicOpEnabled = value;
  return *this;
}
BlendDescriptor& BlendDescriptor::setRenderTarget(unsigned int index, RTBlendDescriptor value)
{
  desc.renderTarget.at(index) = value;
  return *this;
}

GraphicsPipelineDescriptor::GraphicsPipelineDescriptor()
{
  for (int i = 0; i < 8; ++i)
  {
    desc.rtvFormats.at(i) = FormatType::Unknown;
  }
}

void GraphicsPipelineDescriptor::addShader(backend::ShaderType type, std::string path)
{
  auto found = std::find_if(desc.shaders.begin(), desc.shaders.end(), [type](std::pair<backend::ShaderType, std::string>& shader) {
    return shader.first == type;
  });
  if (found != desc.shaders.end())
  {
    found->second = path;
  }
  else
  {
    desc.shaders.push_back(std::make_pair(type, path));
  }
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setVertexShader(std::string path)
{
  addShader(backend::ShaderType::Vertex, path);
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setPixelShader(std::string path)
{
  addShader(backend::ShaderType::Pixel, path);
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setDomainShader(std::string path)
{
  addShader(backend::ShaderType::Domain, path);
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setHullShader(std::string path)
{
  addShader(backend::ShaderType::Hull, path);
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setGeometryShader(std::string path)
{
  addShader(backend::ShaderType::Geometry, path);
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setAmplificationShader(std::string path)
{
  addShader(backend::ShaderType::Amplification, path);
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setMeshShader(std::string path)
{
  addShader(backend::ShaderType::Mesh, path);
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setBlend(BlendDescriptor blend)
{
  desc.blendDesc = blend;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setRasterizer(RasterizerDescriptor rasterizer)
{
  desc.rasterDesc = rasterizer;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setDepthStencil(DepthStencilDescriptor ds)
{
  desc.dsdesc = ds;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setPrimitiveTopology(PrimitiveTopology value)
{
  desc.primitiveTopology = value;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setRenderTargetCount(unsigned int value)
{
  desc.numRenderTargets = value;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setRTVFormat(unsigned int index, FormatType value)
{
  desc.rtvFormats.at(index) = value;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setDSVFormat(FormatType value)
{
  desc.dsvFormat = value;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setMultiSampling(unsigned int SampleCount)
{
  desc.sampleCount = SampleCount;
  return *this;
}
GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::setInterface(PipelineInterfaceDescriptor val)
{
  desc.layout = val;
  return *this;
  }
}