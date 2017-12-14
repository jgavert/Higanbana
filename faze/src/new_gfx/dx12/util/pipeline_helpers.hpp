#pragma once

#include "faze/src/new_gfx/dx12/dx12.hpp"
#include "faze/src/new_gfx/common/pipeline_descriptor.hpp"

namespace faze
{
  namespace backend
  {
    // blend things
    D3D12_BLEND convertBlend(Blend b);
    D3D12_BLEND_OP convertBlendOp(BlendOp op);
    D3D12_LOGIC_OP convertLogicOp(LogicOp op);
    UINT8 convertColorWriteEnable(unsigned colorWrite);

    // raster things
    D3D12_FILL_MODE convertFillMode(FillMode mode);
    D3D12_CULL_MODE convertCullMode(CullMode mode);
    D3D12_CONSERVATIVE_RASTERIZATION_MODE convertConservativeRasterization(ConservativeRasterization mode);

    // depthstencil
    D3D12_DEPTH_WRITE_MASK convertDepthWriteMask(DepthWriteMask mask);
    D3D12_COMPARISON_FUNC convertComparisonFunc(ComparisonFunc func);
    D3D12_STENCIL_OP convertStencilOp(StencilOp op);

    // misc
    D3D12_PRIMITIVE_TOPOLOGY_TYPE convertPrimitiveTopologyType(PrimitiveTopology topology);
	D3D12_PRIMITIVE_TOPOLOGY convertPrimitiveTopology(PrimitiveTopology topology);
  }
}