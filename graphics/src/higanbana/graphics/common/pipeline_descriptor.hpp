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

  RaytracingPipelineDescriptor()
  {
  }

  RaytracingPipelineDescriptor& setRayHitGroup(int index, RayHitGroupDescriptor hitgroup) {
    HIGAN_ASSERT(index >= 0 && index < 64, "sanity check for index");
    if (hitGroups.size() < index) hitGroups.resize(index+1);
    hitGroups[index] = hitgroup;
    return *this;
  }

  RaytracingPipelineDescriptor& setRayGen(std::string path) {
    rayGenerationShader = path;
    return *this;
  }
  RaytracingPipelineDescriptor& setMissShader(std::string path) {
    missShader = path;
    return *this;
  }

  RaytracingPipelineDescriptor& setInterface(PipelineInterfaceDescriptor val) {
    layout = val;
    return *this;
  }
};

  
  class ComputePipelineDescriptor
  {
  public:
    std::string shaderSourcePath;
    std::string rootSignature;
    PipelineInterfaceDescriptor layout;
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

    ComputePipelineDescriptor& setInterface(PipelineInterfaceDescriptor val)
    {
      layout = val;
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

    RTBlendDescriptor()
    {}

    RTBlendDescriptor& setBlendEnable(bool value)
    {
      desc.blendEnable = value;
      return *this;
    }
    /*
    RTBlendDescriptor& setLogicOpEnable(bool value)
    {
      desc.logicOpEnable = value;
      return *this;
    }*/
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
    /*
    RTBlendDescriptor& setLogicOp(LogicOp value)
    {
      desc.logicOp = value;
      return *this;
    }*/
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
    RasterizerDescriptor& setFillMode(FillMode value = FillMode::Solid)
    {
      desc.fill = value;
      return *this;
    }
    RasterizerDescriptor& setCullMode(CullMode value = CullMode::Back)
    {
      desc.cull = value;
      return *this;
    }
    RasterizerDescriptor& setFrontCounterClockwise(bool value = false)
    {
      desc.frontCounterClockwise = value;
      return *this;
    }
    RasterizerDescriptor& setDepthBias(int value = 0)
    {
      desc.depthBias = value;
      return *this;
    }
    RasterizerDescriptor& setDepthBiasClamp(float value = 0.f)
    {
      desc.depthBiasClamp = value;
      return *this;
    }
    RasterizerDescriptor& setSlopeScaledDepthBias(float value = 0.f)
    {
      desc.slopeScaledDepthBias = value;
      return *this;
    }
    RasterizerDescriptor& setDepthClipEnable(bool value = true)
    {
      desc.depthClipEnable = value;
      return *this;
    }
    RasterizerDescriptor& setMultisampleEnable(bool value = false)
    {
      desc.multisampleEnable = value;
      return *this;
    }
    RasterizerDescriptor& setAntialiasedLineEnable(bool value = false)
    {
      desc.antialiasedLineEnable = value;
      return *this;
    }
    RasterizerDescriptor& setForcedSampleCount(unsigned int value = 0)
    {
      desc.forcedSampleCount = value;
      return *this;
    }
    RasterizerDescriptor& setConservativeRaster(ConservativeRasterization value = ConservativeRasterization::Off)
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
    BlendDescriptor& setLogicOp(LogicOp value)
    {
      desc.logicOp = value;
      return *this;
    }
    BlendDescriptor& setLogicOpEnable(bool value)
    {
      desc.logicOpEnabled = value;
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

    GraphicsPipelineDescriptor()
    {
      for (int i = 0; i < 8; ++i)
      {
        desc.rtvFormats.at(i) = FormatType::Unknown;
      }
    }
    private:

    void addShader(backend::ShaderType type, std::string path)
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

    public:

    GraphicsPipelineDescriptor& setVertexShader(std::string path)
    {
      addShader(backend::ShaderType::Vertex, path);
      return *this;
    }
    GraphicsPipelineDescriptor& setPixelShader(std::string path)
    {
      addShader(backend::ShaderType::Pixel, path);
      return *this;
    }
    GraphicsPipelineDescriptor& setDomainShader(std::string path)
    {
      addShader(backend::ShaderType::Domain, path);
      return *this;
    }
    GraphicsPipelineDescriptor& setHullShader(std::string path)
    {
      addShader(backend::ShaderType::Hull, path);
      return *this;
    }
    GraphicsPipelineDescriptor& setGeometryShader(std::string path)
    {
      addShader(backend::ShaderType::Geometry, path);
      return *this;
    }
    GraphicsPipelineDescriptor& setAmplificationShader(std::string path)
    {
      addShader(backend::ShaderType::Amplification, path);
      return *this;
    }
    GraphicsPipelineDescriptor& setMeshShader(std::string path)
    {
      addShader(backend::ShaderType::Mesh, path);
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
    GraphicsPipelineDescriptor& setInterface(PipelineInterfaceDescriptor val)
    {
      desc.layout = val;
      return *this;
    }
  };
}