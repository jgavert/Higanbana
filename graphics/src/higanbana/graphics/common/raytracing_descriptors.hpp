#pragma once
#include "higanbana/graphics/desc/formats.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/buffer.hpp"
#include <higanbana/core/math/math.hpp>
//#include <higanbana/core/system/memview.hpp>
#include <string>
#include <array>
#include <algorithm>

namespace higanbana
{
namespace desc
{
enum RaytracingGeometryFlag : uint32_t {
  Default = 0,
  Opaque = 0x1,
  NoDuplicateAnyHitInvocation = 0x2
};
enum RaytracingASBuildFlags : uint32_t {
  None = 0,
  allowUpdate = 0x1,
  allowCompaction = 0x2,
  preferFastTrace = 0x4,
  preferFastBuild = 0x8,
  minimizeMemory = 0x10,
  performUpdate = 0x20
};

struct RaytracingTriangleDescription {
  ResourceHandle indexBuffer;
  FormatType indexFormat = FormatType::Unknown;
  uint indexByteOffset = 0;
  uint indexCount = 0;

  ResourceHandle vertexBuffer;
  FormatType vertexFormat = FormatType::Unknown;
  uint vertexByteOffset = 0;
  uint vertexCount = 0;
  uint vertexStride = 0;

  ResourceHandle transformBuffer;
  uint transformByteOffset = 0;

  RaytracingGeometryFlag flags;
};

class RaytracingTriangles {
public:
  RaytracingTriangleDescription desc;
  RaytracingTriangles();
  RaytracingTriangles& indices(BufferIBV ibv);
  RaytracingTriangles& vertices(BufferSRV srv);
  RaytracingTriangles& transform(Buffer buf, uint byteoffset);
  RaytracingTriangles& indices(uint count, FormatType format);
  RaytracingTriangles& vertices(uint count, FormatType format);
  RaytracingTriangles& flags(RaytracingGeometryFlag flags);
};

enum class AccelerationStructureType {
  TopLevel,
  BottomLevel
};

class RaytracingAccelerationStructureInputs {
public:
  struct Desc{
    AccelerationStructureType type;
    vector<RaytracingTriangleDescription> triangles;
    ResourceHandle instances;
    uint instancesOffset = 0;
    uint instanceCount = 0;
    RaytracingASBuildFlags mode = RaytracingASBuildFlags::None;
  } desc;
  
  RaytracingAccelerationStructureInputs();
  RaytracingAccelerationStructureInputs& type(AccelerationStructureType type);
  RaytracingAccelerationStructureInputs& instances(Buffer buffer, uint instanceCount, uint byteOffset = 0);
  RaytracingAccelerationStructureInputs& buildMode(RaytracingASBuildFlags value);
  RaytracingAccelerationStructureInputs& push_triangles(RaytracingTriangles value);
  RaytracingAccelerationStructureInputs& insert_triangles(vector<RaytracingTriangles> descs);
};

struct RaytracingASPreBuildInfo {
  uint64_t ResultDataMaxSizeInBytes;
  uint64_t ScratchDataSizeInBytes;
  uint64_t UpdateScratchDataSizeInBytes;
};
}
}