#pragma once
#include "higanbana/graphics/desc/formats.hpp"
#include "higanbana/graphics/desc/shader_desc.hpp"
#include "higanbana/graphics/desc/shader_input_descriptor.hpp"
#include "higanbana/graphics/desc/pipeline_interface_descriptor.hpp"
#include <higanbana/core/math/math.hpp>
#include <string>
#include <array>

namespace higanbana
{

class RayHitGroupDescriptor
{
  public:
  std::string hitGroupIdentifier;
  std::string closestHit;
  std::string anyHit;
  std::string intersection;

  RayHitGroupDescriptor(std::string name = "")
    : hitGroupIdentifier(name)
  {
  }

  RayHitGroupDescriptor& setClosestHit(std::string path)
  {
    closestHit = path;
    return *this;
  }
  RayHitGroupDescriptor& setAnyHit(std::string path)
  {
    anyHit = path;
    return *this;
  }
  RayHitGroupDescriptor& setIntersection(std::string path)
  {
    intersection = path;
    return *this;
  }
};

class RaytracingPipelineDescriptor
{
public:
  vector<RayHitGroupDescriptor> hitGroups;
  std::string rayGenerationShader;
  std::string missShader;
  std::string rootSignature;
  PipelineInterfaceDescriptor layout;

  RaytracingPipelineDescriptor();
  RaytracingPipelineDescriptor& setRayHitGroup(int index, RayHitGroupDescriptor hitgroup);
  RaytracingPipelineDescriptor& setRayGen(std::string path);
  RaytracingPipelineDescriptor& setMissShader(std::string path);
  RaytracingPipelineDescriptor& setInterface(PipelineInterfaceDescriptor val);
};

  
  class ComputePipelineDescriptor
  {
  public:
    std::string shaderSourcePath;
    std::string rootSignature;
    PipelineInterfaceDescriptor layout;
    uint3 shaderGroups;

    ComputePipelineDescriptor();
    const char* shader() const;
    ComputePipelineDescriptor& setShader(std::string path);
    ComputePipelineDescriptor& setThreadGroups(uint3 val);
    ComputePipelineDescriptor& setInterface(PipelineInterfaceDescriptor val);
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
    Red = 1,
    Green = 2,
    Blue = 4,
    Alpha = 8,
    All = (((Red | Green) | Blue) | Alpha)
  };

  class RTBlendDescriptor
  {
  public:
    struct Desc
    {
      bool blendEnable = false;
      //bool logicOpEnable = false;
      Blend srcBlend = Blend::One;
      Blend destBlend = Blend::Zero;
      BlendOp blendOp = BlendOp::Add;
      Blend srcBlendAlpha = Blend::One;
      Blend destBlendAlpha = Blend::Zero;
      BlendOp blendOpAlpha = BlendOp::Add;
      //LogicOp logicOp = LogicOp::Noop;
      unsigned int colorWriteEnable = ColorWriteEnable::All;
    } desc;

    RTBlendDescriptor();

    RTBlendDescriptor& setBlendEnable(bool value);
    /*
    RTBlendDescriptor& setLogicOpEnable(bool value);*/
    RTBlendDescriptor& setSrcBlend(Blend value);
    RTBlendDescriptor& setDestBlend(Blend value);
    RTBlendDescriptor& setBlendOp(BlendOp value);
    RTBlendDescriptor& setSrcBlendAlpha(Blend value);
    RTBlendDescriptor& setDestBlendAlpha(Blend value);
    RTBlendDescriptor& setBlendOpAlpha(BlendOp value);
    /*
    RTBlendDescriptor& setLogicOp(LogicOp value);*/
    RTBlendDescriptor& setColorWriteEnable(ColorWriteEnable value);
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

  enum class ConservativeRasterization
  {
    Off,
    On
  };

  class RasterizerDescriptor
  {
  public:
    struct Desc
    {
      FillMode fill = FillMode::Solid;
      CullMode cull = CullMode::Back;
      bool frontCounterClockwise = false;
      int depthBias = 0;
      float depthBiasClamp = 0.f;
      float slopeScaledDepthBias = 0.f;
      bool depthClipEnable = true;
      bool multisampleEnable = false;
      bool antialiasedLineEnable = false;
      unsigned int forcedSampleCount = 0;
      ConservativeRasterization conservativeRaster = ConservativeRasterization::Off;
    } desc;

    RasterizerDescriptor();
    RasterizerDescriptor& setFillMode(FillMode value = FillMode::Solid);
    RasterizerDescriptor& setCullMode(CullMode value = CullMode::Back);
    RasterizerDescriptor& setFrontCounterClockwise(bool value = false);
    RasterizerDescriptor& setDepthBias(int value = 0);
    RasterizerDescriptor& setDepthBiasClamp(float value = 0.f);
    RasterizerDescriptor& setSlopeScaledDepthBias(float value = 0.f);
    RasterizerDescriptor& setDepthClipEnable(bool value = true);
    RasterizerDescriptor& setMultisampleEnable(bool value = false);
    RasterizerDescriptor& setAntialiasedLineEnable(bool value = false);
    RasterizerDescriptor& setForcedSampleCount(unsigned int value = 0);
    RasterizerDescriptor& setConservativeRaster(ConservativeRasterization value = ConservativeRasterization::Off);
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
    GreaterEqual,
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
  public:
    struct Desc
    {
      StencilOp failOp = StencilOp::Keep;
      StencilOp depthFailOp = StencilOp::Keep;
      StencilOp passOp = StencilOp::Keep;
      ComparisonFunc stencilFunc = ComparisonFunc::Always;
    } desc;

    DepthStencilOpDesc()
    {}

    DepthStencilOpDesc& setFailOp(StencilOp value)
    {
      desc.failOp = value;
      return *this;
    }
    DepthStencilOpDesc& setDepthFailOp(StencilOp value)
    {
      desc.depthFailOp = value;
      return *this;
    }
    DepthStencilOpDesc& setPassOp(StencilOp value)
    {
      desc.passOp = value;
      return *this;
    }
    DepthStencilOpDesc& setStencilFunc(ComparisonFunc value)
    {
      desc.stencilFunc = value;
      return *this;
    }
  };

  class DepthStencilDescriptor
  {
  public:
    struct Desc
    {
      bool depthEnable = true;
      DepthWriteMask depthWriteMask = DepthWriteMask::All;
      ComparisonFunc depthFunc = ComparisonFunc::Less;
      bool stencilEnable = false;
      uint8_t stencilReadMask = 0;
      uint8_t stencilWriteMask = 0;
      DepthStencilOpDesc frontFace;
      DepthStencilOpDesc backFace;
    } desc;

    DepthStencilDescriptor();
    DepthStencilDescriptor& setDepthEnable(bool value);
    DepthStencilDescriptor& setDepthWriteMask(DepthWriteMask value);
    DepthStencilDescriptor& setDepthFunc(ComparisonFunc value);
    DepthStencilDescriptor& setStencilEnable(bool value);
    DepthStencilDescriptor& setStencilReadMask(uint8_t value);
    DepthStencilDescriptor& setStencilWriteMask(uint8_t value);
    DepthStencilDescriptor& setFrontFace(DepthStencilOpDesc value);
    DepthStencilDescriptor& setBackFace(DepthStencilOpDesc value);
  };

  enum class PrimitiveTopology
  {
    Undefined,
    Point,
    Line,
    Triangle,
    Patch
  };

  enum class MultisampleCount
  {
    s1 = 1,
    s2 = 2,
    s4 = 4,
    s8 = 8,
    s16 = 16,
    s32 = 32,
    s64 = 64
  };

  class BlendDescriptor
  {
  public:
    struct Desc
    {
      bool alphaToCoverageEnable = false;
      bool independentBlendEnable = false;
      bool logicOpEnabled = false;
      LogicOp logicOp = LogicOp::Noop;
      std::array<RTBlendDescriptor, 8> renderTarget;
    } desc;

    BlendDescriptor();
    BlendDescriptor& setAlphaToCoverageEnable(bool value);
    BlendDescriptor& setIndependentBlendEnable(bool value);
    BlendDescriptor& setLogicOp(LogicOp value);
    BlendDescriptor& setLogicOpEnable(bool value);
    BlendDescriptor& setRenderTarget(unsigned int index, RTBlendDescriptor value);
  };

  class GraphicsPipelineDescriptor
  {
  public:
    struct Desc
    {
      std::string rootSignature;
      PipelineInterfaceDescriptor layout;
      /*std::string vertexShaderPath;
      std::string pixelShaderPath;
      std::string domainShaderPath;
      std::string hullShaderPath;
      std::string geometryShaderPath;*/
      std::vector<std::pair<backend::ShaderType, std::string>> shaders;
      BlendDescriptor blendDesc;
      RasterizerDescriptor rasterDesc;
      DepthStencilDescriptor dsdesc;
      PrimitiveTopology primitiveTopology = PrimitiveTopology::Triangle;
      unsigned int numRenderTargets = 0;
      std::array<FormatType, 8> rtvFormats;
      FormatType dsvFormat = FormatType::Unknown;
      unsigned int sampleCount = 1;
    } desc;

    GraphicsPipelineDescriptor();
    private:
    void addShader(backend::ShaderType type, std::string path);
    public:
    GraphicsPipelineDescriptor& setVertexShader(std::string path);
    GraphicsPipelineDescriptor& setPixelShader(std::string path);
    GraphicsPipelineDescriptor& setDomainShader(std::string path);
    GraphicsPipelineDescriptor& setHullShader(std::string path);
    GraphicsPipelineDescriptor& setGeometryShader(std::string path);
    GraphicsPipelineDescriptor& setAmplificationShader(std::string path);
    GraphicsPipelineDescriptor& setMeshShader(std::string path);
    GraphicsPipelineDescriptor& setBlend(BlendDescriptor blend);
    GraphicsPipelineDescriptor& setRasterizer(RasterizerDescriptor rasterizer);
    GraphicsPipelineDescriptor& setDepthStencil(DepthStencilDescriptor ds);
    GraphicsPipelineDescriptor& setPrimitiveTopology(PrimitiveTopology value);
    GraphicsPipelineDescriptor& setRenderTargetCount(unsigned int value);
    GraphicsPipelineDescriptor& setRTVFormat(unsigned int index, FormatType value);
    GraphicsPipelineDescriptor& setDSVFormat(FormatType value);
    GraphicsPipelineDescriptor& setMultiSampling(unsigned int SampleCount);
    GraphicsPipelineDescriptor& setInterface(PipelineInterfaceDescriptor val);
  };
}