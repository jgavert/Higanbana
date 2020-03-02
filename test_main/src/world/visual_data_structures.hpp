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
  bool occlusionIndex;
  bool emissiveIndex;
);

struct BufferData
{
  std::string name;
  higanbana::FormatType format;
  higanbana::vector<unsigned char> data;
};

struct MeshData
{
  higanbana::FormatType indiceFormat;
  higanbana::FormatType vertexFormat;
  higanbana::FormatType normalFormat;
  higanbana::FormatType texCoordFormat;
  higanbana::FormatType tangentFormat;
  higanbana::vector<unsigned char> indices;
  higanbana::vector<unsigned char> vertices;
  higanbana::vector<unsigned char> normals;
  higanbana::vector<unsigned char> texCoords;
  higanbana::vector<unsigned char> tangents;
};