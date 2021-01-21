#pragma once
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resources/graphics_api.hpp"
#include <cstdint>
#include <higanbana/core/datastructures/proxy.hpp>

namespace higanbana
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
      AccelerationStructure = 0x200,
      ShadingRateSource = 0x400
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

    const char* toString(AccessUsage stage);
    const char* toString(AccessStage stage);
    const char* toString(TextureLayout stage);
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
        QueueType queue_index : 10;
      };
      uint32_t raw;        
    };

    ResourceState()
      : usage(backend::AccessUsage::Unknown)
      , stage(backend::AccessStage::Common)
      , layout(backend::TextureLayout::Undefined)
      , queue_index(QueueType::Unknown)
    {
    }

    
    ResourceState(backend::AccessUsage usage, backend::AccessStage stage, backend::TextureLayout layout, QueueType queueIndex)
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

  struct ImageBarrier
  {
    ResourceState before;
    ResourceState after;
    ResourceHandle handle;
    uint32_t startMip : 4;
    uint32_t mipSize : 4;
    uint32_t startArr : 12;
    uint32_t arrSize : 12;
  };

  struct BufferBarrier
  {
    ResourceState before;
    ResourceState after;
    ResourceHandle handle;
  };
}