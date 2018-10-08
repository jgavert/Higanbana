#pragma once
#include "faze/src/new_gfx/desc/formats.hpp"
#include "faze/src/new_gfx/common/descriptor_layout.hpp"
#include "core/src/math/math.hpp"
#include <string>
#include <array>

namespace faze
{
  class ComputePipelineDescriptor
  {
  public:
    std::string shaderSourcePath;
    std::string rootSignature;
    backend::DescriptorLayout layout;
    uint3 shaderGroups;

    ComputePipelineDescriptor()
    {
    }

    const char* shader() const
    {
      return shaderSourcePath.c_str();
    }

    ComputePipelineDescriptor& setShader(std::string path)
    {
      shaderSourcePath = path;
      return *this;
    }

    ComputePipelineDescriptor& setThreadGroups(uint3 val)
    {
      shaderGroups = val;
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
      bool logicOpEnable = false;
      Blend srcBlend = Blend::One;
      Blend destBlend = Blend::Zero;
      BlendOp blendOp = BlendOp::Add;
      Blend srcBlendAlpha = Blend::One;
      Blend destBlendAlpha = Blend::Zero;
      BlendOp blendOpAlpha = BlendOp::Add;
      LogicOp logicOp = LogicOp::Noop;
      unsigned int colorWriteEnable = ColorWriteEnable::All;
    } desc;

    RTBlendDescriptor()
    {}

    RTBlendDescriptor& setBlendEnable(bool value)
    {
      desc.blendEnable = value;
      return *this;
    }
    RTBlendDescriptor& setLogicOpEnable(bool value)
    {
      desc.logicOpEnable = value;
      return *this;
    }
    RTBlendDescriptor& setSrcBlend(Blend value)
    {
      desc.srcBlend = value;
      return *this;
    }
    RTBlendDescriptor& setDestBlend(Blend value)
    {
      desc.destBlend = value;
      return *this;
    }
    RTBlendDescriptor& setBlendOp(BlendOp value)
    {
      desc.blendOp = value;
      return *this;
    }
    RTBlendDescriptor& setSrcBlendAlpha(Blend value)
    {
      desc.srcBlendAlpha = value;
      return *this;
    }
    RTBlendDescriptor& setDestBlendAlpha(Blend value)
    {
      desc.destBlendAlpha = value;
      return *this;
    }
    RTBlendDescriptor& setBlendOpAlpha(BlendOp value)
    {
      desc.blendOpAlpha = value;
      return *this;
    }
    RTBlendDescriptor& setLogicOp(LogicOp value)
    {
      desc.logicOp = value;
      return *this;
    }
    RTBlendDescriptor& setColorWriteEnable(ColorWriteEnable value)
    {
      desc.colorWriteEnable = value;
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

    RasterizerDescriptor()
    {}
    RasterizerDescriptor& setFillMode(FillMode value)
    {
      desc.fill = value;
      return *this;
    }
    RasterizerDescriptor& setCullMode(CullMode value)
    {
      desc.cull = value;
      return *this;
    }
    RasterizerDescriptor& setFrontCounterClockwise(bool value)
    {
      desc.frontCounterClockwise = value;
      return *this;
    }
    RasterizerDescriptor& setDepthBias(int value)
    {
      desc.depthBias = value;
      return *this;
    }
    RasterizerDescriptor& setDepthBiasClamp(float value)
    {
      desc.depthBiasClamp = value;
      return *this;
    }
    RasterizerDescriptor& setSlopeScaledDepthBias(float value)
    {
      desc.slopeScaledDepthBias = value;
      return *this;
    }
    RasterizerDescriptor& setDepthClipEnable(bool value)
    {
      desc.depthClipEnable = value;
      return *this;
    }
    RasterizerDescriptor& setMultisampleEnable(bool value)
    {
      desc.multisampleEnable = value;
      return *this;
    }
    RasterizerDescriptor& setAntialiasedLineEnable(bool value)
    {
      desc.antialiasedLineEnable = value;
      return *this;
    }
    RasterizerDescriptor& setForcedSampleCount(unsigned int value)
    {
      desc.forcedSampleCount = value;
      return *this;
    }
    RasterizerDescriptor& setConservativeRaster(ConservativeRasterization value)
    {
      desc.conservativeRaster = value;
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

    DepthStencilDescriptor()
    {
    }

    DepthStencilDescriptor& setDepthEnable(bool value)
    {
      desc.depthEnable = value;
      return *this;
    }
    DepthStencilDescriptor& setDepthWriteMask(DepthWriteMask value)
    {
      desc.depthWriteMask = value;
      return *this;
    }
    DepthStencilDescriptor& setDepthFunc(ComparisonFunc value)
    {
      desc.depthFunc = value;
      return *this;
    }
    DepthStencilDescriptor& setStencilEnable(bool value)
    {
      desc.stencilEnable = value;
      return *this;
    }
    DepthStencilDescriptor& setStencilReadMask(uint8_t value)
    {
      desc.stencilReadMask = value;
      return *this;
    }
    DepthStencilDescriptor& setStencilWriteMask(uint8_t value)
    {
      desc.stencilWriteMask = value;
      return *this;
    }
    DepthStencilDescriptor& setFrontFace(DepthStencilOpDesc value)
    {
      desc.frontFace = value;
      return *this;
    }
    DepthStencilDescriptor& setBackFace(DepthStencilOpDesc value)
    {
      desc.backFace = value;
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

  class BlendDescriptor
  {
  public:
    struct Desc
    {
      bool alphaToCoverageEnable = false;
      bool independentBlendEnable = false;
      std::array<RTBlendDescriptor, 8> renderTarget;
    } desc;

    BlendDescriptor()
    {
      desc.renderTarget.at(0) = RTBlendDescriptor();
    }
    BlendDescriptor& setAlphaToCoverageEnable(bool value)
    {
      desc.alphaToCoverageEnable = value;
      return *this;
    }
    BlendDescriptor& setIndependentBlendEnable(bool value)
    {
      desc.independentBlendEnable = value;
      return *this;
    }
    BlendDescriptor& setRenderTarget(unsigned int index, RTBlendDescriptor value)
    {
      desc.renderTarget.at(index) = value;
      return *this;
    }
  };

  class GraphicsPipelineDescriptor
  {
  public:
    struct Desc
    {
      std::string rootSignature;
      backend::DescriptorLayout layout;
      std::string vertexShaderPath;
      std::string pixelShaderPath;
      std::string domainShaderPath;
      std::string hullShaderPath;
      std::string geometryShaderPath;
      BlendDescriptor blendDesc;
      RasterizerDescriptor rasterDesc;
      DepthStencilDescriptor dsdesc;
      PrimitiveTopology primitiveTopology = PrimitiveTopology::Triangle;
      unsigned int numRenderTargets = 0;
      std::array<FormatType, 8> rtvFormats;
      FormatType dsvFormat = FormatType::Unknown;
      unsigned int sampleCount = 1;
    } desc;

    GraphicsPipelineDescriptor()
    {
      for (int i = 0; i < 8; ++i)
      {
        desc.rtvFormats.at(i) = FormatType::Unknown;
      }
    }

    template <typename Shader>
    GraphicsPipelineDescriptor& setRS()
    {
      rootSignature = Shader::rootSignature();
      return *this;
    }
    GraphicsPipelineDescriptor& setVertexShader(std::string path)
    {
      desc.vertexShaderPath = path;
      return *this;
    }
    GraphicsPipelineDescriptor& setPixelShader(std::string path)
    {
      desc.pixelShaderPath = path;
      return *this;
    }
    GraphicsPipelineDescriptor& setDomainShader(std::string path)
    {
      desc.domainShaderPath = path;
      return *this;
    }
    GraphicsPipelineDescriptor& setHullShader(std::string path)
    {
      desc.hullShaderPath = path;
      return *this;
    }
    GraphicsPipelineDescriptor& setGeometryShader(std::string path)
    {
      desc.geometryShaderPath = path;
      return *this;
    }
    GraphicsPipelineDescriptor& setBlend(BlendDescriptor blend)
    {
      desc.blendDesc = blend;
      return *this;
    }
    GraphicsPipelineDescriptor& setRasterizer(RasterizerDescriptor rasterizer)
    {
      desc.rasterDesc = rasterizer;
      return *this;
    }
    GraphicsPipelineDescriptor& setDepthStencil(DepthStencilDescriptor ds)
    {
      desc.dsdesc = ds;
      return *this;
    }
    GraphicsPipelineDescriptor& setPrimitiveTopology(PrimitiveTopology value)
    {
      desc.primitiveTopology = value;
      return *this;
    }
    GraphicsPipelineDescriptor& setRenderTargetCount(unsigned int value)
    {
      desc.numRenderTargets = value;
      return *this;
    }
    GraphicsPipelineDescriptor& setRTVFormat(unsigned int index, FormatType value)
    {
      desc.rtvFormats.at(index) = value;
      return *this;
    }
    GraphicsPipelineDescriptor& setDSVFormat(FormatType value)
    {
      desc.dsvFormat = value;
      return *this;
    }
    GraphicsPipelineDescriptor& setMultiSampling(unsigned int SampleCount)
    {
      desc.sampleCount = SampleCount;
      return *this;
    }
  };
}