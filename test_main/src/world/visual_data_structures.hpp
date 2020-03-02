#pragma once

#include <higanbana/core/math/math.hpp>
#include <higanbana/graphics/desc/formats.hpp>
#include <higanbana/graphics/common/cpuimage.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/graphics/desc/shader_input_descriptor.hpp>

struct InstanceDraw
{
  float3 wp;
  float4x4 mat;
  int meshId;
  int materialId;
};

struct ActiveCamera
{
  float3 position;
  quaternion direction;
  float fov;
  float minZ;
  float maxZ;
};

struct TextureData
{
  higanbana::CpuImage image;
};

/*
struct PBRMetallicRoughness
{
  double baseColorFactor[4];
  int baseColorTexIndex;
  double metallicFactor;
  double roughnessFactor;
  int metallicRoughnessTexIndex;
};

struct MaterialData
{
  double emissiveFactor[3];
  double alphaCutoff;
  bool doubleSided;
  PBRMetallicRoughness pbr;

  bool hadNormalTex;
  bool hadOcclusionTexture;
  bool hadEmissiveTexture;
};*/

SHADER_STRUCT(MaterialData,
  double3 emissiveFactor;
  double alphaCutoff;
  bool doubleSided;
  // pbr
  double4 baseColorFactor;
  double metallicFactor;
  double roughnessFactor;

  uint albedoIndex; // baseColorTexIndex
  uint normalIndex; // normalTexIndex
  uint metallicRoughnessIndex;
  uint occlusionIndex;
  uint emissiveIndex;
);

struct BufferData
{
  std::string name;
  higanbana::vector<unsigned char> data;
};

struct BufferAccessor
{
  higanbana::Id buffer;
  size_t offset;
  size_t size;
  higanbana::FormatType format;
};

struct MeshData
{
  BufferAccessor indices;
  BufferAccessor vertices;
  BufferAccessor normals;
  BufferAccessor texCoords;
  BufferAccessor tangents;
};