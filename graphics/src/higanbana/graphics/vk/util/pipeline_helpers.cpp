#include "higanbana/graphics/vk/util/pipeline_helpers.hpp"
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
  namespace backend
  {
    vk::BlendFactor convertBlend(Blend b)
    {
      switch (b)
      {
      case Blend::Zero:
        return vk::BlendFactor::eZero;
      case Blend::One:
        return vk::BlendFactor::eOne;
      case Blend::SrcColor:
        return vk::BlendFactor::eSrcColor;
      case Blend::InvSrcColor:
        return vk::BlendFactor::eOneMinusSrcColor;
      case Blend::SrcAlpha:
        return vk::BlendFactor::eSrcAlpha;
      case Blend::InvSrcAlpha:
        return vk::BlendFactor::eOneMinusSrcAlpha;
      case Blend::DestAlpha:
        return vk::BlendFactor::eDstAlpha;
      case Blend::InvDestAlpha:
        return vk::BlendFactor::eOneMinusDstAlpha;
      case Blend::DestColor:
        return vk::BlendFactor::eDstColor;
      case Blend::InvDestColor:
        return vk::BlendFactor::eOneMinusDstColor;
      case Blend::SrcAlphaSat:
        return vk::BlendFactor::eSrcAlphaSaturate;
      case Blend::BlendFactor: // TODO: what, same as below
        return vk::BlendFactor::eOne;
      case Blend::InvBlendFactor: // TODO: what, dynamic bled state
        return vk::BlendFactor::eZero;
      case Blend::Src1Color:
        return vk::BlendFactor::eSrc1Color;
      case Blend::InvSrc1Color:
        return vk::BlendFactor::eOneMinusSrc1Color;
      case Blend::Src1Alpha:
        return vk::BlendFactor::eSrc1Alpha;
      case Blend::InvSrc1Alpha:
      default:
        break;
      }
      return vk::BlendFactor::eOneMinusSrc1Alpha;
    }

    vk::BlendOp convertBlendOp(BlendOp op)
    {
      switch (op)
      {
      case BlendOp::Add:
        return vk::BlendOp::eAdd;
      case BlendOp::Subtract:
        return vk::BlendOp::eSubtract;
      case BlendOp::RevSubtract:
        return vk::BlendOp::eReverseSubtract;
      case BlendOp::Min:
        return vk::BlendOp::eMin;
      case BlendOp::Max:
      default:
        break;
      }
      return vk::BlendOp::eMax;
    }

    vk::LogicOp convertLogicOp(LogicOp op)
    {
      switch (op)
      {
      case LogicOp::Clear:
        return vk::LogicOp::eClear;
      case LogicOp::Set:
        return vk::LogicOp::eSet;
      case LogicOp::Copy:
        return vk::LogicOp::eCopy;
      case LogicOp::CopyInverted:
        return vk::LogicOp::eCopyInverted;
      case LogicOp::Noop:
        return vk::LogicOp::eNoOp;
      case LogicOp::Invert:
        return vk::LogicOp::eInvert;
      case LogicOp::AND:
        return vk::LogicOp::eAnd;
      case LogicOp::NAND:
        return vk::LogicOp::eNand;
      case LogicOp::OR:
        return vk::LogicOp::eOr;
      case LogicOp::NOR:
        return vk::LogicOp::eNor;
      case LogicOp::XOR:
        return vk::LogicOp::eXor;
      case LogicOp::Equiv:
        return vk::LogicOp::eEquivalent;
      case LogicOp::ANDReverse:
        return vk::LogicOp::eAndReverse;
      case LogicOp::ANDInverted:
        return vk::LogicOp::eAndInverted;
      case LogicOp::ORReverse:
        return vk::LogicOp::eOrReverse;
      case LogicOp::ORInverted:
      default:
        break;
      }
      return vk::LogicOp::eOrInverted;
    }

    vk::ColorComponentFlags convertColorWriteEnable(unsigned colorWrite)
    {
      vk::ColorComponentFlags mask;
      if ((colorWrite & ColorWriteEnable::Red) > 0)
      {
        mask |= vk::ColorComponentFlagBits::eR;
      }
      if ((colorWrite & ColorWriteEnable::Blue) > 0)
      {
        mask |= vk::ColorComponentFlagBits::eB;
      }
      if ((colorWrite & ColorWriteEnable::Green) > 0)
      {
        mask |= vk::ColorComponentFlagBits::eG;
      }
      if ((colorWrite & ColorWriteEnable::Alpha) > 0)
      {
        mask |= vk::ColorComponentFlagBits::eA;
      }
      return mask;
    }

    vk::PolygonMode convertFillMode(FillMode mode)
    {
      switch (mode)
      {
      case FillMode::Solid:
        return vk::PolygonMode::eFill;
      case FillMode::Wireframe:
      default:
        break;
      }
      return vk::PolygonMode::eLine;
    }

    vk::CullModeFlags convertCullMode(CullMode mode)
    {
      switch (mode)
      {
      case CullMode::Back:
        return vk::CullModeFlagBits::eBack;
      case CullMode::Front:
        return vk::CullModeFlagBits::eFront;
      case CullMode::None:
      default:
        break;
      }
      return vk::CullModeFlagBits::eNone;
    }

    vk::ConservativeRasterizationModeEXT convertConservativeRasterization(ConservativeRasterization mode)
    {
      if (mode == ConservativeRasterization::On)
        return vk::ConservativeRasterizationModeEXT::eOverestimate;
      return vk::ConservativeRasterizationModeEXT::eDisabled;
    }

/*
    vk::Depth convertDepthWriteMask(DepthWriteMask mask)
    {
      if (mask == DepthWriteMask::Zero)
        return D3D12_DEPTH_WRITE_MASK_ZERO;
      return D3D12_DEPTH_WRITE_MASK_ALL;
    }
    */
    vk::CompareOp convertComparisonFunc(ComparisonFunc func)
    {
      switch (func)
      {
      case ComparisonFunc::Never:
        return vk::CompareOp::eNever;
      case ComparisonFunc::Less:
        return vk::CompareOp::eLess;
      case ComparisonFunc::Equal:
        return vk::CompareOp::eEqual;
      case ComparisonFunc::LessEqual:
        return vk::CompareOp::eLessOrEqual;
      case ComparisonFunc::Greater:
        return vk::CompareOp::eGreater;
      case ComparisonFunc::NotEqual:
        return vk::CompareOp::eNotEqual;
      case ComparisonFunc::GreaterEqual:
        return vk::CompareOp::eGreaterOrEqual;
      case ComparisonFunc::Always:
      default:
        break;
      }
      return vk::CompareOp::eAlways;
    }
    /*
    eKeep = VK_STENCIL_OP_KEEP,
    eZero = VK_STENCIL_OP_ZERO,
    eReplace = VK_STENCIL_OP_REPLACE,
    eIncrementAndClamp = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
    eDecrementAndClamp = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
    eInvert = VK_STENCIL_OP_INVERT,
    eIncrementAndWrap = VK_STENCIL_OP_INCREMENT_AND_WRAP,
    eDecrementAndWrap = VK_STENCIL_OP_DECREMENT_AND_WRAP
    */

    vk::StencilOp convertStencilOp(StencilOp op)
    {
      switch (op)
      {
      case StencilOp::Keep:
        return vk::StencilOp::eKeep;
      case StencilOp::Zero:
        return vk::StencilOp::eZero;
      case StencilOp::Replace:
        return vk::StencilOp::eReplace;
      case StencilOp::IncrSat:
        return vk::StencilOp::eIncrementAndClamp;
      case StencilOp::DecrSat:
        return vk::StencilOp::eDecrementAndClamp;
      case StencilOp::Invert:
        return vk::StencilOp::eInvert;
      case StencilOp::Incr:
        return vk::StencilOp::eIncrementAndWrap;
      case StencilOp::Decr:
      default:
        break;
      }
      return vk::StencilOp::eDecrementAndWrap;
    }

/*
    vk::PolygonMode convertPrimitiveTopologyType(PrimitiveTopology topology)
    {
      switch (topology)
      {
      case PrimitiveTopology::Undefined:
        return vk::PolygonMode::eFill;
      case PrimitiveTopology::Point:
        return vk::PolygonMode::ePoint;
      case PrimitiveTopology::Line:
        return vk::PolygonMode::eLine;
      case PrimitiveTopology::Triangle:
        return vk::PolygonMode::eFill;
      case PrimitiveTopology::Patch:
        HIGAN_ASSERT(false, "don't know what to do.");
      default:
        break;
      }
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
    */

    vk::PrimitiveTopology convertPrimitiveTopology(PrimitiveTopology topology)
    {
      switch (topology)
      {
      case PrimitiveTopology::Undefined:
        return vk::PrimitiveTopology::eLineList;
      case PrimitiveTopology::Point:
        return vk::PrimitiveTopology::ePointList;
      case PrimitiveTopology::Line:
        return vk::PrimitiveTopology::eLineList;
      case PrimitiveTopology::Triangle:
        return vk::PrimitiveTopology::eTriangleList;
      case PrimitiveTopology::Patch:
      default:
        break;
      }
      return vk::PrimitiveTopology::ePatchList;
    }

    vk::SampleCountFlags convertSamplecount(MultisampleCount count)
    {
      switch (count)
      {
        case MultisampleCount::s1:
          return vk::SampleCountFlagBits::e1;
        case MultisampleCount::s2:
          return vk::SampleCountFlagBits::e2;
        case MultisampleCount::s4:
          return vk::SampleCountFlagBits::e4;
        case MultisampleCount::s8:
          return vk::SampleCountFlagBits::e8;
        case MultisampleCount::s16:
          return vk::SampleCountFlagBits::e16;
        case MultisampleCount::s32:
          return vk::SampleCountFlagBits::e32;
        case MultisampleCount::s64:
        default:
          break;
      }
      return vk::SampleCountFlagBits::e64; 
    }

    vector<vk::PipelineColorBlendAttachmentState> getBlendAttachments(const BlendDescriptor& desc, int renderTargetCount)
    {
      vector<vk::PipelineColorBlendAttachmentState> states(renderTargetCount);
      for (int i = 0; i < renderTargetCount; ++i)
      {
        auto& rtb = desc.desc.renderTarget[i].desc;
        vk::PipelineColorBlendAttachmentState state;
        state = state.setBlendEnable(rtb.blendEnable); // Is blending enabled for attachment
        state = state.setSrcColorBlendFactor(convertBlend(rtb.srcBlend));
        state = state.setDstColorBlendFactor(convertBlend(rtb.destBlend));
        state = state.setColorBlendOp(convertBlendOp(rtb.blendOp));
        state = state.setSrcAlphaBlendFactor(convertBlend(rtb.srcBlendAlpha));
        state = state.setDstAlphaBlendFactor(convertBlend(rtb.destBlendAlpha));
        state = state.setAlphaBlendOp(convertBlendOp(rtb.blendOpAlpha));
        state = state.setColorWriteMask(convertColorWriteEnable(rtb.colorWriteEnable));
        // differences to DX12
        //desc.BlendState.RenderTarget[i].LogicOpEnable = rtb.logicOpEnable;
        //desc.BlendState.RenderTarget[i].LogicOp = convertLogicOp(rtb.logicOp);
        states[i] = state;
      }
      return states;
    }

    vk::PipelineColorBlendStateCreateInfo getBlendStateDesc(const BlendDescriptor& desc, const vector<vk::PipelineColorBlendAttachmentState>& attachments)
    {
      vk::PipelineColorBlendStateCreateInfo blendstate  = vk::PipelineColorBlendStateCreateInfo()
      .setAttachmentCount(attachments.size())
      .setPAttachments(attachments.data())
      .setLogicOp(convertLogicOp(desc.desc.logicOp))
      .setLogicOpEnable(desc.desc.logicOpEnabled);
      return blendstate;
    }

    vk::PipelineRasterizationStateCreateInfo getRasterStateDesc(const RasterizerDescriptor& desc)
    {
      vk::PipelineRasterizationStateCreateInfo rasterState;
      {
        // Rasterization
        auto& rd = desc.desc;
        HIGAN_ASSERT(rd.conservativeRaster != higanbana::ConservativeRasterization::On, "Conservative raster ext code not written.");
        rasterState = rasterState.setDepthClampEnable(rd.depthBiasClamp)
          .setRasterizerDiscardEnable(false) // apparently discards everything immediately
          .setPolygonMode(vk::PolygonMode::eLine)
          .setCullMode(convertCullMode(rd.cull))
          .setFrontFace(vk::FrontFace::eClockwise)
          .setDepthBiasEnable(bool(rd.depthBias))
          .setDepthBiasClamp(rd.depthBiasClamp)
          .setDepthBiasSlopeFactor(rd.slopeScaledDepthBias);

        if (rd.fill == FillMode::Solid)
        {
          rasterState = rasterState.setPolygonMode(vk::PolygonMode::eFill);
        }
        if (rd.frontCounterClockwise)
        {
          rasterState = rasterState.setFrontFace(vk::FrontFace::eCounterClockwise);
        }
          //.setDepthBiasConstantFactor(float(rd.depthBias))
      }
      return rasterState;
    }
/*
        desc.RasterizerState.FillMode = convertFillMode(rd.fill);
        desc.RasterizerState.CullMode = convertCullMode(rd.cull);
        desc.RasterizerState.FrontCounterClockwise = rd.frontCounterClockwise;
        desc.RasterizerState.DepthBias = rd.depthBias;
        desc.RasterizerState.DepthBiasClamp = rd.depthBiasClamp;
        desc.RasterizerState.SlopeScaledDepthBias = rd.slopeScaledDepthBias;
        desc.RasterizerState.DepthClipEnable = rd.depthClipEnable;
        desc.RasterizerState.MultisampleEnable = rd.multisampleEnable;
        desc.RasterizerState.AntialiasedLineEnable = rd.antialiasedLineEnable;
        desc.RasterizerState.ForcedSampleCount = rd.forcedSampleCount;
        desc.RasterizerState.ConservativeRaster = convertConservativeRasterization(rd.conservativeRaster);
      }
      */
     vk::PipelineDepthStencilStateCreateInfo getDepthStencilDesc(const DepthStencilDescriptor& desc)
     {
      vk::PipelineDepthStencilStateCreateInfo depthstencil;
      {
        // DepthStencil
        auto& dss = desc.desc;
        /*
        VkBool32 depthWriteEnable;
        VkBool32 depthBoundsTestEnable;
        float minDepthBounds;
        float maxDepthBounds;
        */
        depthstencil = depthstencil.setDepthTestEnable(dss.depthEnable);
        depthstencil = depthstencil.setDepthWriteEnable(dss.depthEnable); // TODO: separate written depth from testing
        depthstencil = depthstencil.setDepthCompareOp(convertComparisonFunc(dss.depthFunc));
        depthstencil = depthstencil.setDepthBoundsTestEnable(dss.depthEnable); // TODO: fix/expose
        depthstencil = depthstencil.setStencilTestEnable(dss.stencilEnable);

        /*
        uint32_t compareMask; // TODO: figure out how these work
        uint32_t writeMask;
        uint32_t reference;
        */
        vk::StencilOpState front, back;
        front = front.setFailOp(convertStencilOp(dss.frontFace.desc.failOp))
          .setPassOp(convertStencilOp(dss.frontFace.desc.passOp))
          .setDepthFailOp(convertStencilOp(dss.frontFace.desc.depthFailOp))
          .setCompareOp(convertComparisonFunc(dss.frontFace.desc.stencilFunc));
        back = back.setFailOp(convertStencilOp(dss.backFace.desc.failOp))
          .setPassOp(convertStencilOp(dss.backFace.desc.passOp))
          .setDepthFailOp(convertStencilOp(dss.backFace.desc.depthFailOp))
          .setCompareOp(convertComparisonFunc(dss.backFace.desc.stencilFunc));
        depthstencil = depthstencil.setFront(front);
        depthstencil = depthstencil.setBack(back);
      }
      return depthstencil;
     }
    vk::PipelineInputAssemblyStateCreateInfo getInputAssemblyDesc(const GraphicsPipelineDescriptor& desc)
    {
      vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
      inputAssembly = inputAssembly.setPrimitiveRestartEnable(false);
      inputAssembly = inputAssembly.setTopology(convertPrimitiveTopology(desc.desc.primitiveTopology));
      return inputAssembly;
    }
    vk::PipelineMultisampleStateCreateInfo getMultisampleDesc(const GraphicsPipelineDescriptor& desc)
    {
      return vk::PipelineMultisampleStateCreateInfo()
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setSampleShadingEnable(false)
        .setMinSampleShading(0.f)
        .setAlphaToCoverageEnable(desc.desc.blendDesc.desc.alphaToCoverageEnable)
        .setAlphaToOneEnable(false);
    }
  }
}