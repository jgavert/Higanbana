#include "higanbana/graphics/dx12/util/raytracing_helpers.hpp"

#if defined(HIGANBANA_PLATFORM_WINDOWS)
namespace higanbana
{
namespace backend
{
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS rtASBuildMode(desc::RaytracingASBuildFlags mode){
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ret{};
  auto getFlag = [mode](desc::RaytracingASBuildFlags flag, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS targetFlag) {
    if (mode & flag == flag) {
      return targetFlag;
    }
    return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
  };

  ret |= getFlag(desc::RaytracingASBuildFlags::allowUpdate, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE);
  ret |= getFlag(desc::RaytracingASBuildFlags::allowCompaction, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION);
  ret |= getFlag(desc::RaytracingASBuildFlags::preferFastTrace, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
  ret |= getFlag(desc::RaytracingASBuildFlags::preferFastBuild, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD);
  ret |= getFlag(desc::RaytracingASBuildFlags::minimizeMemory, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY);
  ret |= getFlag(desc::RaytracingASBuildFlags::performUpdate, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE);
  return ret;
}
}
}
#endif