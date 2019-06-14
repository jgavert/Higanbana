#pragma once
#include <cstdint>

namespace faze
{
  namespace backend
  {
    // Only one state is allowed
    enum class AccessUsage : uint32_t 
    {
      Unknown,
      Read,
      Write,
      ReadWrite
    };

    // all stages that I support. could probably invest in more states than just "graphics" if need arises.
    enum AccessStage : uint32_t 
    {
      Compute = 0,
      Graphics = 0x1,
      Transfer = 0x2,
      Index = 0x4,
      Indirect = 0x8,
      Rendertarget = 0x10,
      DepthStencil = 0x20,
      Present = 0x40,
      Raytrace = 0x80,
      AccelerationStructure = 0x100
    };

    // carbon copy from vulkan
    enum class TextureLayout : uint32_t
    {
      Undefined,
      General,
      Rendertarget,
      DepthStencil,
      DepthStencilReadOnly,
      ShaderReadOnly,
      TransferSrc,
      TransferDst,
      Preinitialize,
      DepthReadOnlyStencil,
      StencilReadOnlyDepth,
      Present,
      SharedPresent,
      ShadingRateNV,
      FragmentDensityMap
    };
  }

  struct ResourceState
  {
    union
    {
      struct
      {
        backend::AccessUsage usage : 2;
        backend::AccessStage stage : 16;
        backend::TextureLayout layout : 4;
        uint32_t queue_index : 10;
      };
      uint32_t raw;        
    };
  };
}