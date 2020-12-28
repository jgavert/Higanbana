#pragma once

#include <higanbana/core/math/math.hpp>
#include <higanbana/graphics/desc/formats.hpp>
#include <higanbana/graphics/common/cpuimage.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/graphics/desc/shader_input_descriptor.hpp>
#include <higanbana/core/entity/database.hpp>

struct InstanceDraw
{
  float3 wp;
  float4x4 mat;
  int meshId;
  int materialId;
};
struct ChunkBlockDraw 
{
  float3 position;
  unsigned int materialIndex;
};

struct ActiveCamera
{
  float3 position;
  quaternion direction;
  float fov;
  float minZ;
  float maxZ;
  float aperture;
  float focusDist;
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
  float3 emissiveFactor;
  float alphaCutoff;
  // pbr
  float4 baseColorFactor;
  float metallicFactor;
  float roughnessFactor;

  uint albedoIndex; // baseColorTexIndex
  uint normalIndex; // normalTexIndex
  uint metallicRoughnessIndex;
  uint occlusionIndex;
  uint emissiveIndex;
  uint doubleSided;
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