#pragma once
#include <cstdint>
#include <core/datastructures/proxy.hpp>

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
      Common = 0,
      Compute = 0x1,
      Graphics = 0x2,
      Transfer = 0x4,
      Index = 0x8,
      Indirect = 0x10,
      Rendertarget = 0x20,
      DepthStencil = 0x40,
      Present = 0x80,
      Raytrace = 0x100,
      AccelerationStructure = 0x200
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

    ResourceState()
      : usage(backend::AccessUsage::Unknown)
      , stage(backend::AccessStage::Common)
      , layout(backend::TextureLayout::Undefined)
      , queue_index(0)
    {
    }

    
    ResourceState(backend::AccessUsage usage, backend::AccessStage stage, backend::TextureLayout layout, uint32_t queueIndex)
      : usage(usage)
      , stage(stage)
      , layout(layout)
      , queue_index(queueIndex)
    {
    }
  };

  struct TextureResourceState
  {
    int mips;
    vector<ResourceState> states;
  };
}