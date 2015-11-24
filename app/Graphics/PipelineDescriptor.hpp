#pragma once
#include "Descriptors\Formats.hpp"
#include <d3d12.h>
#include <string>
#include <array>

class ComputePipelineDescriptor
{
	friend class GpuDevice;

  std::string shaderSourcePath;
  std::string rootDescPath;
public:
	ComputePipelineDescriptor()
	{

	}

  ComputePipelineDescriptor& RootDesc(std::string path)
  {
    rootDescPath = path;
    return *this;
  }

  ComputePipelineDescriptor& shader(std::string path)
  {
    shaderSourcePath = path;
    return *this;
  }

};

enum class Blend
{
  Zero = D3D12_BLEND_ZERO,
  One = D3D12_BLEND_ONE,
  SrcColor = D3D12_BLEND_SRC_COLOR,
  InvSrcColor = D3D12_BLEND_INV_SRC_COLOR,
  SrcAlpha = D3D12_BLEND_SRC_ALPHA,
  InvSrcAlpha = D3D12_BLEND_INV_SRC_ALPHA,
  DestAlpha = D3D12_BLEND_DEST_ALPHA,
  InvDestAlpha = D3D12_BLEND_INV_DEST_ALPHA,
  DestColor = D3D12_BLEND_DEST_COLOR,
  InvDestColor = D3D12_BLEND_INV_DEST_COLOR,
  SrcAlphaSat = D3D12_BLEND_SRC_ALPHA_SAT,
  BlendFactor = D3D12_BLEND_BLEND_FACTOR,
  InvBlendFactor = D3D12_BLEND_INV_BLEND_FACTOR,
  Src1Color = D3D12_BLEND_SRC1_COLOR,
  InvSrc1Color = D3D12_BLEND_INV_SRC1_COLOR,
  Src1Alpha = D3D12_BLEND_SRC1_ALPHA,
  InvSrc1Alpha = D3D12_BLEND_INV_SRC1_ALPHA
};

enum class BlendOp
{
  Add = D3D12_BLEND_OP_ADD,
  Subtract = D3D12_BLEND_OP_SUBTRACT,
  RevSubtract = D3D12_BLEND_OP_REV_SUBTRACT,
  Min = D3D12_BLEND_OP_MIN,
  Max = D3D12_BLEND_OP_MAX
};

enum class LogicOp
{
  Clear = D3D12_LOGIC_OP_CLEAR,
  Set = D3D12_LOGIC_OP_SET,
  Copy = D3D12_LOGIC_OP_COPY,
  CopyInverted = D3D12_LOGIC_OP_COPY_INVERTED,
  Noop = D3D12_LOGIC_OP_NOOP,
  Invert = D3D12_LOGIC_OP_INVERT,
  AND = D3D12_LOGIC_OP_AND,
  NAND = D3D12_LOGIC_OP_NAND,
  OR = D3D12_LOGIC_OP_OR,
  NOR = D3D12_LOGIC_OP_NOR,
  XOR = D3D12_LOGIC_OP_XOR,
  Equiv = D3D12_LOGIC_OP_EQUIV,
  ANDReverse = D3D12_LOGIC_OP_AND_REVERSE,
  ANDInverted = D3D12_LOGIC_OP_AND_INVERTED,
  ORReverse = D3D12_LOGIC_OP_OR_REVERSE,
  ORInverted = D3D12_LOGIC_OP_OR_INVERTED
};

enum ColorWriteEnable
{
  Red = D3D12_COLOR_WRITE_ENABLE_RED,
  Green = D3D12_COLOR_WRITE_ENABLE_GREEN,
  Blue = D3D12_COLOR_WRITE_ENABLE_BLUE,
  Alpha = D3D12_COLOR_WRITE_ENABLE_ALPHA,
  All = D3D12_COLOR_WRITE_ENABLE_ALL
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

  D3D12_RENDER_TARGET_BLEND_DESC getDesc()
  {
    D3D12_RENDER_TARGET_BLEND_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.BlendEnable = blendEnable;
    desc.LogicOpEnable = logicOpEnable;
    desc.SrcBlend = static_cast<D3D12_BLEND>(srcBlend);
    desc.DestBlend = static_cast<D3D12_BLEND>(destBlend);
    desc.BlendOp = static_cast<D3D12_BLEND_OP>(blendOp);
    desc.SrcBlendAlpha = static_cast<D3D12_BLEND>(srcBlendAlpha);
    desc.DestBlendAlpha = static_cast<D3D12_BLEND>(destBlendAlpha);
    desc.BlendOpAlpha = static_cast<D3D12_BLEND_OP>(blendOpAlpha);
    desc.LogicOp = static_cast<D3D12_LOGIC_OP>(logicOp);
    desc.RenderTargetWriteMask = colorWriteEnable;
    return desc;
  }
public:
  RenderTargetBlendDescriptor(): 
    blendEnable(false), logicOpEnable(false), colorWriteEnable(ColorWriteEnable::All),
    srcBlend(Blend::One), destBlend(Blend::Zero), blendOp(BlendOp::Add),
    srcBlendAlpha(Blend::One), destBlendAlpha(Blend::Zero),  blendOpAlpha(BlendOp::Add), logicOp(LogicOp::Noop){}
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
  Wireframe = D3D12_FILL_MODE_WIREFRAME,
  Solid = D3D12_FILL_MODE_SOLID
};

enum class CullMode
{
  None = D3D12_CULL_MODE_NONE,
  Front = D3D12_CULL_MODE_FRONT,
  Back = D3D12_CULL_MODE_BACK
};

enum class ConvervativeRasterization
{
  Off = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
  On = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON
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

  D3D12_RASTERIZER_DESC getDesc()
  {
    D3D12_RASTERIZER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.FillMode = static_cast<D3D12_FILL_MODE>(fill);
    desc.CullMode = static_cast<D3D12_CULL_MODE>(cull);
    desc.FrontCounterClockwise = frontCounterClockwise;
    desc.DepthBias = depthBias;
    desc.DepthBiasClamp = depthBiasClamp;
    desc.SlopeScaledDepthBias = slopeScaledDepthBias;
    desc.DepthClipEnable = depthClipEnable;
    desc.MultisampleEnable = multisampleEnable;
    desc.AntialiasedLineEnable = antialiasedLineEnable;
    desc.ForcedSampleCount = forcedSampleCount;
    desc.ConservativeRaster = static_cast<D3D12_CONSERVATIVE_RASTERIZATION_MODE>(conservativeRaster);

    return desc;
  }

public:
  RasterizerDescriptor(): frontCounterClockwise(false), depthBias(0), depthBiasClamp(0.f), slopeScaledDepthBias(0.f),
    depthClipEnable(true), multisampleEnable(false), antialiasedLineEnable(false), forcedSampleCount(0), conservativeRaster(ConvervativeRasterization::Off),
    fill(FillMode::Solid), cull(CullMode::Back)
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
  Zero = D3D12_DEPTH_WRITE_MASK_ZERO,
  All = D3D12_DEPTH_WRITE_MASK_ALL
};

enum class ComparisonFunc
{
  Never =  D3D12_COMPARISON_FUNC_NEVER,
  Less = D3D12_COMPARISON_FUNC_LESS,
  Equal = D3D12_COMPARISON_FUNC_EQUAL,
  LessEqual = D3D12_COMPARISON_FUNC_LESS_EQUAL,
  Greater = D3D12_COMPARISON_FUNC_GREATER,
  NotEqual = D3D12_COMPARISON_FUNC_NOT_EQUAL,
  GreaterEqual = D3D12_COMPARISON_FUNC_GREATER_EQUAL,
  Always = D3D12_COMPARISON_FUNC_ALWAYS
};

enum class StencilOp
{
  Keep = D3D12_STENCIL_OP_KEEP,
  Zero = D3D12_STENCIL_OP_ZERO,
  Replace = D3D12_STENCIL_OP_REPLACE,
  IncrSat = D3D12_STENCIL_OP_INCR_SAT,
  DecrSat = D3D12_STENCIL_OP_DECR_SAT,
  Invert = D3D12_STENCIL_OP_INVERT,
  Incr = D3D12_STENCIL_OP_INCR,
  Decr = D3D12_STENCIL_OP_DECR
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

  D3D12_DEPTH_STENCILOP_DESC getDesc()
  {
    D3D12_DEPTH_STENCILOP_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.StencilFailOp = static_cast<D3D12_STENCIL_OP>(failOp);
    desc.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(depthFailOp);
    desc.StencilPassOp = static_cast<D3D12_STENCIL_OP>(passOp);
    desc.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(stencilFunc);

    return desc;
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

  D3D12_DEPTH_STENCIL_DESC getDesc()
  {
    D3D12_DEPTH_STENCIL_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.DepthEnable = depthEnable;
    desc.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(depthWriteMask);
    desc.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(depthFunc);
    desc.StencilEnable = stencilEnable;
    desc.StencilReadMask = stencilReadMask;
    desc.StencilWriteMask = stencilWriteMask;
    desc.FrontFace = frontFace.getDesc();
    desc.BackFace = backFace.getDesc();
    return desc;
  }
public:
  DepthStencilDescriptor() :depthEnable(true), depthWriteMask(DepthWriteMask::All), depthFunc(ComparisonFunc::Less),
    stencilEnable(false), stencilReadMask(D3D12_DEFAULT_STENCIL_READ_MASK), stencilWriteMask(D3D12_DEFAULT_STENCIL_WRITE_MASK)
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
  Undefined = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,
  Point = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
  Line = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
  Triangle = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
  Patch = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH
};

class GraphicsBlendDescriptor
{
private:
  friend class GpuDevice;
  friend class GraphicsPipelineDescriptor;

  bool alphaToCoverageEnable;
  bool independentBlendEnable;
  std::array<RenderTargetBlendDescriptor, 8> renderTarget;
  D3D12_BLEND_DESC getDesc()
  {
    D3D12_BLEND_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.AlphaToCoverageEnable = alphaToCoverageEnable;
    desc.IndependentBlendEnable = independentBlendEnable;
    for (int i = 0; i < 8; ++i)
    {
      desc.RenderTarget[i] = renderTarget.at(i).getDesc();
    }
    return desc;
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

  D3D12_GRAPHICS_PIPELINE_STATE_DESC getDesc()
  {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
    ZeroMemory(&desc, sizeof(desc));


    desc.BlendState = blendDesc.getDesc();
    desc.RasterizerState = rasterDesc.getDesc();
    desc.DepthStencilState = dsdesc.getDesc();
    desc.NumRenderTargets = numRenderTargets;
    desc.SampleDesc.Count = sampleCount;
    desc.SampleDesc.Quality = sampleQuality;
    for (int i = 0;i < 8;++i)
    {
      desc.RTVFormats[i] = FormatToDXGIFormat[rtvFormats.at(i)];
    }
    desc.DSVFormat = FormatToDXGIFormat[dsvFormat];
    desc.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(primitiveTopology);
    
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    desc.SampleMask = UINT_MAX;
    desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    /*
    // Not needed ... ?!
    desc.StreamOutput
    desc.InputLayout
    */

    return desc;
  }

public:
  GraphicsPipelineDescriptor()
  : numRenderTargets(0), sampleCount(1), sampleQuality(0), dsvFormat(FormatType::Unknown), primitiveTopology(PrimitiveTopology::Triangle)
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
