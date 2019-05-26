#include "graphics/vk/util/pipeline_helpers.hpp"
#include <core/global_debug.hpp>

namespace faze
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
      vk::ColorComponentFlags mask = 0;
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
        F_ASSERT(false, "don't know what to do.");
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
  }
}