#include "faze/src/new_gfx/dx12/util/pipeline_helpers.hpp"

namespace faze
{
  namespace backend
  {
    D3D12_BLEND convertBlend(Blend b)
    {
      switch (b)
      {
      case Blend::Zero:
        return D3D12_BLEND_ZERO;
      case Blend::One:
        return D3D12_BLEND_ONE;
      case Blend::SrcColor:
        return D3D12_BLEND_SRC_COLOR;
      case Blend::InvSrcColor:
        return D3D12_BLEND_INV_SRC_COLOR;
      case Blend::SrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
      case Blend::InvSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
      case Blend::DestAlpha:
        return D3D12_BLEND_DEST_ALPHA;
      case Blend::InvDestAlpha:
        return D3D12_BLEND_INV_DEST_ALPHA;
      case Blend::DestColor:
        return D3D12_BLEND_DEST_COLOR;
      case Blend::InvDestColor:
        return D3D12_BLEND_INV_DEST_COLOR;
      case Blend::SrcAlphaSat:
        return D3D12_BLEND_SRC_ALPHA_SAT;
      case Blend::BlendFactor:
        return D3D12_BLEND_BLEND_FACTOR;
      case Blend::InvBlendFactor:
        return D3D12_BLEND_INV_BLEND_FACTOR;
      case Blend::Src1Color:
        return D3D12_BLEND_SRC1_COLOR;
      case Blend::InvSrc1Color:
        return D3D12_BLEND_INV_SRC1_COLOR;
      case Blend::Src1Alpha:
        return D3D12_BLEND_SRC1_ALPHA;
      case Blend::InvSrc1Alpha:
      default:
        break;
      }
      return D3D12_BLEND_INV_SRC1_ALPHA;
    }

    D3D12_BLEND_OP convertBlendOp(BlendOp op)
    {
      switch (op)
      {
      case BlendOp::Add:
        return D3D12_BLEND_OP_ADD;
      case BlendOp::Subtract:
        return D3D12_BLEND_OP_SUBTRACT;
      case BlendOp::RevSubtract:
        return D3D12_BLEND_OP_REV_SUBTRACT;
      case BlendOp::Min:
        return D3D12_BLEND_OP_MIN;
      case BlendOp::Max:
      default:
        break;
      }
      return D3D12_BLEND_OP_MAX;
    }

    D3D12_LOGIC_OP convertLogicOp(LogicOp op)
    {
      switch (op)
      {
      case LogicOp::Clear:
        return D3D12_LOGIC_OP_CLEAR;
      case LogicOp::Set:
        return D3D12_LOGIC_OP_SET;
      case LogicOp::Copy:
        return D3D12_LOGIC_OP_COPY;
      case LogicOp::CopyInverted:
        return D3D12_LOGIC_OP_COPY_INVERTED;
      case LogicOp::Noop:
        return D3D12_LOGIC_OP_NOOP;
      case LogicOp::Invert:
        return D3D12_LOGIC_OP_INVERT;
      case LogicOp::AND:
        return D3D12_LOGIC_OP_AND;
      case LogicOp::NAND:
        return D3D12_LOGIC_OP_NAND;
      case LogicOp::OR:
        return D3D12_LOGIC_OP_OR;
      case LogicOp::NOR:
        return D3D12_LOGIC_OP_NOR;
      case LogicOp::XOR:
        return D3D12_LOGIC_OP_XOR;
      case LogicOp::Equiv:
        return D3D12_LOGIC_OP_EQUIV;
      case LogicOp::ANDReverse:
        return D3D12_LOGIC_OP_AND_REVERSE;
      case LogicOp::ANDInverted:
        return D3D12_LOGIC_OP_AND_INVERTED;
      case LogicOp::ORReverse:
        return D3D12_LOGIC_OP_OR_REVERSE;
      case LogicOp::ORInverted:
      default:
        break;
      }
      return D3D12_LOGIC_OP_OR_INVERTED;
    }

    UINT8 convertColorWriteEnable(unsigned colorWrite)
    {
      UINT8 mask = 0;
      if ((colorWrite & ColorWriteEnable::Red) > 0)
      {
        mask |= D3D12_COLOR_WRITE_ENABLE_RED;
      }
      if ((colorWrite & ColorWriteEnable::Blue) > 0)
      {
        mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
      }
      if ((colorWrite & ColorWriteEnable::Green) > 0)
      {
        mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
      }
      if ((colorWrite & ColorWriteEnable::Alpha) > 0)
      {
        mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
      }
      return mask;
    }

    D3D12_FILL_MODE convertFillMode(FillMode mode)
    {
      switch (mode)
      {
      case FillMode::Solid:
        return D3D12_FILL_MODE_SOLID;
      case FillMode::Wireframe:
      default:
        break;
      }
      return D3D12_FILL_MODE_WIREFRAME;
    }

    D3D12_CULL_MODE convertCullMode(CullMode mode)
    {
      switch (mode)
      {
      case CullMode::Back:
        return D3D12_CULL_MODE_BACK;
      case CullMode::Front:
        return D3D12_CULL_MODE_FRONT;
      case CullMode::None:
      default:
        break;
      }
      return D3D12_CULL_MODE_NONE;
    }

    D3D12_CONSERVATIVE_RASTERIZATION_MODE convertConservativeRasterization(ConservativeRasterization mode)
    {
      if (mode == ConservativeRasterization::On)
        return D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
      return D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    D3D12_DEPTH_WRITE_MASK convertDepthWriteMask(DepthWriteMask mask)
    {
      if (mask == DepthWriteMask::Zero)
        return D3D12_DEPTH_WRITE_MASK_ZERO;
      return D3D12_DEPTH_WRITE_MASK_ALL;
    }

    D3D12_COMPARISON_FUNC convertComparisonFunc(ComparisonFunc func)
    {
      switch (func)
      {
      case ComparisonFunc::Never:
        return D3D12_COMPARISON_FUNC_NEVER;
      case ComparisonFunc::Less:
        return D3D12_COMPARISON_FUNC_LESS;
      case ComparisonFunc::Equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
      case ComparisonFunc::LessEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
      case ComparisonFunc::Greater:
        return D3D12_COMPARISON_FUNC_GREATER;
      case ComparisonFunc::NotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
      case ComparisonFunc::GreaterEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
      case ComparisonFunc::Always:
      default:
        break;
      }
      return D3D12_COMPARISON_FUNC_ALWAYS;
    }

    D3D12_STENCIL_OP convertStencilOp(StencilOp op)
    {
      switch (op)
      {
      case StencilOp::Keep:
        return D3D12_STENCIL_OP_KEEP;
      case StencilOp::Zero:
        return D3D12_STENCIL_OP_ZERO;
      case StencilOp::Replace:
        return D3D12_STENCIL_OP_REPLACE;
      case StencilOp::IncrSat:
        return D3D12_STENCIL_OP_INCR_SAT;
      case StencilOp::DecrSat:
        return D3D12_STENCIL_OP_DECR_SAT;
      case StencilOp::Invert:
        return D3D12_STENCIL_OP_INVERT;
      case StencilOp::Incr:
        return D3D12_STENCIL_OP_INCR;
      case StencilOp::Decr:
      default:
        break;
      }
      return D3D12_STENCIL_OP_DECR;
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE convertPrimitiveTopology(PrimitiveTopology topology)
    {
      switch (topology)
      {
      case PrimitiveTopology::Undefined:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
      case PrimitiveTopology::Point:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
      case PrimitiveTopology::Line:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
      case PrimitiveTopology::Triangle:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      case PrimitiveTopology::Patch:
      default:
        break;
      }
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
  }
}