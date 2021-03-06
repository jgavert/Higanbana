#include "higanbana/graphics/common/raytracing_descriptors.hpp"
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
namespace desc
{
bool validIndexFormat(FormatType format) {
  switch (format){
  case FormatType::Unknown:
  case FormatType::Uint16:
  case FormatType::Uint32:
    return true;
  default:
    return false;
  }
}
bool validVertexFormat(FormatType format) {
  switch (format){
  case FormatType::Float32RG:
  case FormatType::Float32RGB:
  case FormatType::Float16RG:
  case FormatType::Float16RGBA:
  // case SNorm16RG:
  // case SNorm16RGBA:
    return true;
  default:
    return false;
  }
}
RaytracingTriangles::RaytracingTriangles(){}

RaytracingTriangles& RaytracingTriangles::indices(BufferIBV ibv) {
  int elements = ibv.viewDesc().m_elementCount;
  if (elements == -1)
    elements = ibv.desc().desc.width;
  desc.indexCount = elements;
  FormatType format = ibv.viewDesc().m_format;
  if (format == FormatType::Unknown) {
    format = ibv.desc().desc.format;
  }
  HIGAN_ASSERT(validIndexFormat(format), "format must be one of supported.");
  desc.indexFormat = format;
  desc.indexByteOffset = ibv.viewDesc().m_firstElement * formatSizeInfo(desc.indexFormat).pixelSize;
  desc.indexBuffer = ibv.buffer().handle();
  HIGAN_ASSERT(desc.indexBuffer.id != ResourceHandle::InvalidId, "Must be valid ResourceHandle");
  return *this;
}

RaytracingTriangles& RaytracingTriangles::vertices(BufferSRV srv) {
  int elements = srv.viewDesc().m_elementCount;
  if (elements == -1)
    elements = srv.desc().desc.width;
  desc.vertexCount = elements;
  FormatType format = srv.viewDesc().m_format;
  if (format == FormatType::Unknown) {
    format = srv.desc().desc.format;
  }

  HIGAN_ASSERT(validVertexFormat(format), "format must be one of supported.");
  desc.vertexFormat = format;
  desc.vertexByteOffset = srv.viewDesc().m_firstElement * formatSizeInfo(desc.vertexFormat).pixelSize;
  desc.vertexBuffer = srv.buffer().handle();
  desc.vertexStride = srv.desc().desc.stride;
  HIGAN_ASSERT(desc.vertexBuffer.id != ResourceHandle::InvalidId, "Must be valid ResourceHandle");
  return *this;
}

RaytracingTriangles& RaytracingTriangles::transform(Buffer buf, uint byteoffset) {
  desc.transformBuffer = buf.handle();
  desc.transformByteOffset = byteoffset;
  HIGAN_ASSERT(desc.transformBuffer.id != ResourceHandle::InvalidId, "Must be valid ResourceHandle");
  return *this;
}

RaytracingTriangles& RaytracingTriangles::indices(uint count, FormatType format) {
  desc.indexCount = count;
  HIGAN_ASSERT(format != FormatType::Unknown, "format can't be unknown");
  desc.indexFormat = format;
  return *this;
}
RaytracingTriangles& RaytracingTriangles::vertices(uint count, FormatType format) {
  desc.vertexCount = count;
  HIGAN_ASSERT(format != FormatType::Unknown, "format can't be unknown");
  desc.vertexFormat = format;
  desc.vertexStride = formatSizeInfo(desc.vertexFormat).pixelSize;
  return *this;
}
RaytracingTriangles& RaytracingTriangles::flags(RaytracingGeometryFlag flags) {
  desc.flags = flags;
  return *this;
}
RaytracingAccelerationStructureInputs::RaytracingAccelerationStructureInputs() {}

RaytracingAccelerationStructureInputs& RaytracingAccelerationStructureInputs::type(AccelerationStructureType type) {
  desc.type = type;
  return *this;
}

RaytracingAccelerationStructureInputs& RaytracingAccelerationStructureInputs::instances(Buffer buffer, uint instanceCount, uint byteOffset) {
  HIGAN_ASSERT(buffer.handle().id != ResourceHandle::InvalidId, "Must be valid buffer");
  desc.instances = buffer.handle();
  desc.instancesOffset = byteOffset;
  desc.instanceCount = instanceCount;
  return *this;
}

RaytracingAccelerationStructureInputs& RaytracingAccelerationStructureInputs::buildMode(RaytracingASBuildFlags value) {
  desc.mode = value;
  return *this;
}

RaytracingAccelerationStructureInputs& RaytracingAccelerationStructureInputs::push_triangles(RaytracingTriangles value) {
  desc.triangles.push_back(value.desc);
  return *this;
}
RaytracingAccelerationStructureInputs& RaytracingAccelerationStructureInputs::insert_triangles(vector<RaytracingTriangles> descs) {
  for (auto&& d : descs) {
    desc.triangles.push_back(d.desc);
  }
  return *this;
}
}
}