#pragma once
#include <higanbana/core/platform/definitions.hpp>
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/dx12/dx12.hpp"

namespace higanbana
{
  namespace backend
  {
    namespace dx12
    {
      D3D12_SHADER_RESOURCE_VIEW_DESC getSRV(ResourceDescriptor& desc, ShaderViewDescriptor& view);
      D3D12_UNORDERED_ACCESS_VIEW_DESC getUAV(ResourceDescriptor& desc, ShaderViewDescriptor& view);
      D3D12_RENDER_TARGET_VIEW_DESC getRTV(ResourceDescriptor& desc, ShaderViewDescriptor& view);
      D3D12_DEPTH_STENCIL_VIEW_DESC getDSV(ResourceDescriptor& desc, ShaderViewDescriptor& view);
    }
  }
}
#endif