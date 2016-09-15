#pragma once
#include "Descriptors/Formats.hpp"
#include <string>
#include <array>

class ComputePipelineDescriptor
{
	friend class GpuDevice;

  std::string shaderSourcePath;
public:
	ComputePipelineDescriptor()
	{

	}

  const char* shader() const
  {
    return shaderSourcePath.c_str();
  }

  ComputePipelineDescriptor& shader(std::string path)
  {
    shaderSourcePath = path;
    return *this;
  }

};

enum class Blend
{
  Zero,
  One,
  SrcColor,
  InvSrcColor,
  SrcAlpha,
  InvSrcAlpha,
  DestAlpha,
  InvDestAlpha,
  DestColor,
  InvDestColor,
  SrcAlphaSat,
  BlendFactor,
  InvBlendFactor,
  Src1Color,
  InvSrc1Color,
  Src1Alpha,
  InvSrc1Alpha
};

enum class BlendOp
{
  Add,
  Subtract,
  RevSubtract,
  Min,
  Max
};

enum class LogicOp
{
  Clear,
  Set,
  Copy,
  CopyInverted,
  Noop,
  Invert,
  AND,
  NAND,
  OR,
  NOR,
  XOR,
  Equiv,
  ANDReverse,
  ANDInverted,
  ORReverse,
  ORInverted
};

enum ColorWriteEnable
{
  Red,
  Green,
  Blue,
  Alpha,
  All
};

class RenderTargetBlendDescriptor
{
private:
  friend class GpuDevice;
  friend class GraphicsBlendDescriptor;
  bool blendEnable;
  bool logicOpEnable;
  Blend srcBlend;
  Blend destBlend;
  BlendOp blendOp;
  Blend srcBlendAlpha;
  Blend destBlendAlpha;
  BlendOp blendOpAlpha;
  LogicOp logicOp;
  unsigned int colorWriteEnable;

  int getDesc()
  {;
    return 1;
  }
public:
  RenderTargetBlendDescriptor()
    :blendEnable(false)
    ,logicOpEnable(false)
    ,srcBlend(Blend::One)
    ,destBlend(Blend::Zero)
    ,blendOp(BlendOp::Add)
    ,srcBlendAlpha(Blend::One)
    ,destBlendAlpha(Blend::Zero)
    ,blendOpAlpha(BlendOp::Add)
    ,logicOp(LogicOp::Noop)
    ,colorWriteEnable(ColorWriteEnable::All)
  {}

  RenderTargetBlendDescriptor& BlendEnable(bool value)
  {
    blendEnable = value;
    return *this;
  }
  RenderTargetBlendDescriptor& LogicOpEnable(bool value)
  {
    logicOpEnable = value;
    return *this;
  }
  RenderTargetBlendDescriptor& SrcBlend(Blend value)
  {
    srcBlend = value;
    return *this;
  }
  RenderTargetBlendDescriptor& DestBlend(Blend value)
  {
    destBlend = value;
    return *this;
  }
  RenderTargetBlendDescriptor& setBlendOp(BlendOp value)
  {
    blendOp = value;
    return *this;
  }
  RenderTargetBlendDescriptor& SrcBlendAlpha(Blend value)
  {
    srcBlendAlpha = value;
    return *this;
  }
  RenderTargetBlendDescriptor& DestBlendAlpha(Blend value)
  {
    destBlendAlpha = value;
    return *this;
  }
  RenderTargetBlendDescriptor& BlendOpAlpha(BlendOp value)
  {
    blendOpAlpha = value;
    return *this;
  }
  RenderTargetBlendDescriptor& LogicOp(LogicOp value)
  {
    logicOp = value;
    return *this;
  }
  RenderTargetBlendDescriptor& ColorWriteEnable(ColorWriteEnable value)
  {
    colorWriteEnable = value;
    return *this;
  }
};

enum class FillMode
{
  Wireframe,
  Solid
};

enum class CullMode
{
  None,
  Front,
  Back
};

enum class ConvervativeRasterization
{
  Off,
  On
};

class RasterizerDescriptor
{
private:
  friend class GpuDevice;
  friend class GraphicsPipelineDescriptor;
  FillMode fill;
  CullMode cull;
  bool frontCounterClockwise;
  int depthBias;
  float depthBiasClamp;
  float slopeScaledDepthBias;
  bool depthClipEnable;
  bool multisampleEnable;
  bool antialiasedLineEnable;
  unsigned int forcedSampleCount;
  ConvervativeRasterization conservativeRaster;

  int getDesc()
  {
    return 1;
  }

public:
  RasterizerDescriptor()
    : fill(FillMode::Solid)
    , cull(CullMode::Back)
    , frontCounterClockwise(false)
    , depthBias(0)
    , depthBiasClamp(0.f)
    , slopeScaledDepthBias(0.f)
    , depthClipEnable(true)
    , multisampleEnable(false)
    , antialiasedLineEnable(false)
    , forcedSampleCount(0)
    , conservativeRaster(ConvervativeRasterization::Off)
  {}
  RasterizerDescriptor& FillMode(FillMode value)
  {
    fill = value;
    return *this;
  }
  RasterizerDescriptor& CullMode(CullMode value)
  {
    cull = value;
    return *this;
  }
  RasterizerDescriptor& FrontCounterClockwise(bool value)
  {
    frontCounterClockwise = value;
    return *this;
  }
  RasterizerDescriptor& DepthBias(int value)
  {
    depthBias = value;
    return *this;
  }
  RasterizerDescriptor& DepthBiasClamp(float value)
  {
    depthBiasClamp = value;
    return *this;
  }
  RasterizerDescriptor& SlopeScaledDepthBias(float value)
  {
    slopeScaledDepthBias = value;
    return *this;
  }
  RasterizerDescriptor& DepthClipEnable(bool value)
  {
    depthClipEnable = value;
    return *this;
  }
  RasterizerDescriptor& MultisampleEnable(bool value)
  {
    multisampleEnable = value;
    return *this;
  }
  RasterizerDescriptor& AntialiasedLineEnable(bool value)
  {
    antialiasedLineEnable = value;
    return *this;
  }
  RasterizerDescriptor& ForcedSampleCount(unsigned int value)
  {
    forcedSampleCount = value;
    return *this;
  }
  RasterizerDescriptor& ConservativeRaster(ConvervativeRasterization value)
  {
    conservativeRaster = value;
    return *this;
  }
};

enum class DepthWriteMask
{
  Zero,
  All
};

enum class ComparisonFunc
{
  Never,
  Less,
  Equal,
  LessEqual,
  Greater,
  NotEqual,
  GreaterEqualL,
  Always
};

enum class StencilOp
{
  Keep,
  Zero,
  Replace,
  IncrSat,
  DecrSat,
  Invert,
  Incr,
  Decr
};

class DepthStencilOpDesc
{
private:
  friend class GpuDevice;
  friend class DepthStencilDescriptor;
  StencilOp failOp;
  StencilOp depthFailOp;
  StencilOp passOp;
  ComparisonFunc stencilFunc;

  int getDesc()
  {
    return 1;
  }

public:
  DepthStencilOpDesc():failOp(StencilOp::Keep), depthFailOp(StencilOp::Keep), passOp(StencilOp::Keep), stencilFunc(ComparisonFunc::Always)  {}
  DepthStencilOpDesc& FailOp(StencilOp value)
  {
    failOp = value;
    return *this;
  }
  DepthStencilOpDesc& DepthFailOp(StencilOp value)
  {
    depthFailOp = value;
    return *this;
  }
  DepthStencilOpDesc& PassOp(StencilOp value)
  {
    passOp = value;
    return *this;
  }
  DepthStencilOpDesc& StencilFunc(ComparisonFunc value)
  {
    stencilFunc = value;
    return *this;
  }
};

class DepthStencilDescriptor
{
private:
  friend class GpuDevice;
  friend class GraphicsPipelineDescriptor;
  bool depthEnable;
  DepthWriteMask depthWriteMask;
  ComparisonFunc depthFunc;
  bool stencilEnable;
  uint8_t stencilReadMask;
  uint8_t stencilWriteMask;
  DepthStencilOpDesc frontFace;
  DepthStencilOpDesc backFace;

  int getDesc()
  {
    return 1;
  }
public:
  DepthStencilDescriptor() :depthEnable(true), depthWriteMask(DepthWriteMask::All), depthFunc(ComparisonFunc::Less),
    stencilEnable(false), stencilReadMask(0), stencilWriteMask(0)
  {
  }


  DepthStencilDescriptor& DepthEnable(bool value)
  {
    depthEnable = value;
    return *this;
  }
  DepthStencilDescriptor& DepthWriteMask(DepthWriteMask value)
  {
    depthWriteMask = value;
    return *this;
  }
  DepthStencilDescriptor& DepthFunc(ComparisonFunc value)
  {
    depthFunc = value;
    return *this;
  }
  DepthStencilDescriptor& StencilEnable(bool value)
  {
    stencilEnable = value;
    return *this;
  }
  DepthStencilDescriptor& StencilReadMask(uint8_t value)
  {
    stencilReadMask = value;
    return *this;
  }
  DepthStencilDescriptor& StencilWriteMask(uint8_t value)
  {
    stencilWriteMask = value;
    return *this;
  }
  DepthStencilDescriptor& FrontFace(DepthStencilOpDesc value)
  {
    frontFace = value;
    return *this;
  }
  DepthStencilDescriptor& BackFace(DepthStencilOpDesc value)
  {
    backFace = value;
    return *this;
  }
};

enum class PrimitiveTopology
{
  Undefined,
  Point,
  Line,
  Triangle,
  Patch
};

class GraphicsBlendDescriptor
{
private:
  friend class GpuDevice;
  friend class GraphicsPipelineDescriptor;

  bool alphaToCoverageEnable;
  bool independentBlendEnable;
  std::array<RenderTargetBlendDescriptor, 8> renderTarget;
  int getDesc()
  {
    return 1;
  }
public:
  GraphicsBlendDescriptor():
    alphaToCoverageEnable(false),
    independentBlendEnable(false)
  {
    renderTarget.at(0) = RenderTargetBlendDescriptor();
  }
  GraphicsBlendDescriptor& AlphaToCoverageEnable(bool value)
  {
    alphaToCoverageEnable = value;
    return *this;
  }
  GraphicsBlendDescriptor& IndependentBlendEnable(bool value)
  {
    independentBlendEnable = value;
    return *this;
  }
  GraphicsBlendDescriptor& setRenderTarget(unsigned int index, RenderTargetBlendDescriptor value)
  {
    renderTarget.at(index) = value;
    return *this;
  }

};

class GraphicsPipelineDescriptor
{
  friend class GpuDevice;
  std::string rootDescPath;
  std::string vertexShaderPath;
  std::string pixelShaderPath;
  std::string domainShaderPath;
  std::string hullShaderPath;
  std::string geometryShaderPath;
  GraphicsBlendDescriptor blendDesc;
  RasterizerDescriptor rasterDesc;
  DepthStencilDescriptor dsdesc;
  PrimitiveTopology primitiveTopology;
  unsigned int numRenderTargets;
  std::array<FormatType, 8> rtvFormats;
  FormatType dsvFormat;
  unsigned int sampleCount;
  unsigned int sampleQuality;

  int getDesc()
  {
    return 1;
  }

public:
  GraphicsPipelineDescriptor()
  : primitiveTopology(PrimitiveTopology::Triangle)
  , numRenderTargets(0)
  , dsvFormat(FormatType::Unknown)
  , sampleCount(1)
  , sampleQuality(0)
  {
    for (int i = 0;i < 8;++i)
    {
      rtvFormats.at(i) = FormatType::Unknown;
    }
  }

  GraphicsPipelineDescriptor& RootDesc(std::string path)
  {
    rootDescPath = path;
    return *this;
  }
  GraphicsPipelineDescriptor& VertexShader(std::string path)
  {
    vertexShaderPath = path;
    return *this;
  }
  GraphicsPipelineDescriptor& PixelShader(std::string path)
  {
    pixelShaderPath = path;
    return *this;
  }
  GraphicsPipelineDescriptor& DomainShader(std::string path)
  {
    domainShaderPath = path;
    return *this;
  }
  GraphicsPipelineDescriptor& HullShader(std::string path)
  {
    hullShaderPath = path;
    return *this;
  }
  GraphicsPipelineDescriptor& GeometryShader(std::string path)
  {
    geometryShaderPath = path;
    return *this;
  }
  GraphicsPipelineDescriptor& Blend(GraphicsBlendDescriptor desc)
  {
    blendDesc = desc;
    return *this;
  }
  GraphicsPipelineDescriptor& Rasterizer(RasterizerDescriptor desc)
  {
    rasterDesc = desc;
    return *this;
  }
  GraphicsPipelineDescriptor& DepthStencil(DepthStencilDescriptor desc)
  {
    dsdesc = desc;
    return *this;
  }
  GraphicsPipelineDescriptor& PrimitiveTopology(PrimitiveTopology value)
  {
    primitiveTopology = value;
    return *this;
  }
  GraphicsPipelineDescriptor& setRenderTargetCount(unsigned int value)
  {
    numRenderTargets = value;
    return *this;
  }
  GraphicsPipelineDescriptor& RTVFormat(unsigned int index, FormatType value)
  {
    rtvFormats.at(index) = value;
    return *this;
  }
  GraphicsPipelineDescriptor& DSVFormat(FormatType value)
  {
    dsvFormat = value;
    return *this;
  }
  GraphicsPipelineDescriptor& MultiSampling(unsigned int SampleCount, unsigned int SampleQuality)
  {
    sampleCount = SampleCount;
    sampleQuality = SampleQuality;
    return *this;
  }
};
