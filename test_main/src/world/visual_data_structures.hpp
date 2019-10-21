#pragma once

#include <higanbana/core/math/math.hpp>
#include <higanbana/graphics/desc/formats.hpp>
#include <higanbana/core/datastructures/proxy.hpp>

struct InstanceDraw
{
  float3 wp;
  float4x4 mat;
  int meshId;
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