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
  RaytracingTriangles(){}

  RaytracingTriangles& indices(BufferIBV ibv) {
    int elements = ibv.viewDesc().m_elementCount;
    if (elements == -1)
      elements = ibv.desc().desc.width;
    desc.indexCount = elements;
    FormatType format = ibv.viewDesc().m_format;
    if (format == FormatType::Unknown) {
      format = ibv.desc().desc.format;
    }
    HIGAN_ASSERT(format != FormatType::Unknown, "format can't be unknown");
    desc.indexFormat = format;
    desc.indexByteOffset = ibv.viewDesc().m_firstElement * formatSizeInfo(desc.indexFormat).pixelSize;
    desc.indexBuffer = ibv.buffer().handle();
    return *this;
  }
  RaytracingTriangles& vertices(BufferSRV srv) {
    int elements = srv.viewDesc().m_elementCount;
    if (elements == -1)
      elements = srv.desc().desc.width;
    desc.vertexCount = elements;
    FormatType format = srv.viewDesc().m_format;
    if (format == FormatType::Unknown) {
      format = srv.desc().desc.format;
    }
    HIGAN_ASSERT(format != FormatType::Unknown, "format can't be unknown");
    desc.vertexFormat = format;
    desc.vertexByteOffset = srv.viewDesc().m_firstElement * formatSizeInfo(desc.vertexFormat).pixelSize;
    desc.vertexBuffer = srv.buffer().handle();
    desc.vertexStride = srv.desc().desc.stride;
    return *this;
  }
  RaytracingTriangles& transform(Buffer buf, uint byteoffset) {
    desc.transformBuffer = buf.handle();
    desc.transformByteOffset = byteoffset;
    return *this;
  }

  RaytracingTriangles& indices(uint count, FormatType format) {
    desc.indexCount = count;
    HIGAN_ASSERT(format != FormatType::Unknown, "format can't be unknown");
    desc.indexFormat = format;
    return *this;
  }
  RaytracingTriangles& vertices(uint count, FormatType format) {
    desc.vertexCount = count;
    HIGAN_ASSERT(format != FormatType::Unknown, "format can't be unknown");
    desc.vertexFormat = format;
    desc.vertexStride = formatSizeInfo(desc.vertexFormat).pixelSize;
    return *this;
  }
  RaytracingTriangles& flags(RaytracingGeometryFlag flags) {
    desc.flags = flags;
    return *this;
  }
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
    RaytracingASBuildFlags mode = RaytracingASBuildFlags::None;
  } desc;
  
  RaytracingAccelerationStructureInputs() {}

  RaytracingAccelerationStructureInputs& type(AccelerationStructureType type) {
    desc.type = type;
    return *this;
  }

  RaytracingAccelerationStructureInputs& buildMode(RaytracingASBuildFlags value) {
    desc.mode = value;
    return *this;
  }

  RaytracingAccelerationStructureInputs& push_triangles(RaytracingTriangles value) {
    desc.triangles.push_back(value.desc);
    return *this;
  }
  RaytracingAccelerationStructureInputs& insert_triangles(vector<RaytracingTriangles> descs) {
    for (auto&& d : descs) {
      desc.triangles.push_back(d.desc);
    }
    return *this;
  }
};

struct RaytracingASPreBuildInfo {
  uint64_t ResultDataMaxSizeInBytes;
  uint64_t ScratchDataSizeInBytes;
  uint64_t UpdateScratchDataSizeInBytes;
};
}
}