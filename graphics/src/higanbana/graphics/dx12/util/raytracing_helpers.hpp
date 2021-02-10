#pragma once
#include <higanbana/core/platform/definitions.hpp>
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/dx12.hpp"
#include "higanbana/graphics/common/raytracing_descriptors.hpp"

namespace higanbana
{
namespace backend
{
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS rtASBuildMode(desc::RaytracingASBuildFlags mode);
}
}
#endif