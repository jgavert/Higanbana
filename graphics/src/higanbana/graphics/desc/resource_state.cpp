#include "higanbana/graphics/desc/resource_state.hpp"

namespace higanbana
{
  namespace backend
  {
    const char* toString(AccessUsage usage)
    {
      switch (usage)
      {
        case AccessUsage::Unknown: return "Unknown";
        case AccessUsage::Read: return "Read";
        case AccessUsage::ReadWrite: return "ReadWrite";
        case AccessUsage::Write:
        default:  return "Write";
      }
    }
    const char* toString(TextureLayout layout)
    {
      switch (layout)
      {
        case TextureLayout::Undefined:
          return "Undefined";
        case TextureLayout::General:
          return "General";
        case TextureLayout::Rendertarget:
          return "Rendertarget";
        case TextureLayout::DepthStencil:
          return "DepthStencil";
        case TextureLayout::DepthStencilReadOnly:
          return "DepthStencilReadOnly";
        case TextureLayout::ShaderReadOnly:
          return "ShaderReadOnly";
        case TextureLayout::TransferSrc:
          return "TransferSrc";
        case TextureLayout::TransferDst:
          return "TransferDst";
        case TextureLayout::Preinitialize:
          return "Preinitialize";
        case TextureLayout::DepthReadOnlyStencil:
          return "DepthReadOnlyStencil";
        case TextureLayout::StencilReadOnlyDepth:
          return "StencilReadOnlyDepth";
        case TextureLayout::Present:
          return "Present";
        case TextureLayout::SharedPresent:
          return "SharedPresent";
        case TextureLayout::ShadingRateNV:
          return "ShadingRateNV";
        case TextureLayout::FragmentDensityMap:
          return "FragmentDensityMap";
        default:
        break;
      }
      return "Unknown layout";
    }

    const char* toString(AccessStage stage)
    {
      switch (stage)
      {
        case Common:
          return "Common";
        case Compute:
          return "Compute";
        case Graphics:
          return "Graphics";
        case Transfer:
          return "Transfer";
        case Index:
          return "Index";
        case Indirect:
          return "Indirect";
        case Rendertarget:
          return "Rendertarget";
        case DepthStencil:
          return "DepthStencil";
        case Present:
          return "Present";
        case Raytrace:
          return "Raytrace";
        case AccelerationStructure:
          return "AccelerationStructure";
        default:
        break;
      }
      return "Unknown stage";
    }
  }
}