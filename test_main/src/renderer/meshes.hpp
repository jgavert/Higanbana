#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>
#include "../world/visual_data_structures.hpp"

namespace app
{
struct MeshViews
{
  higanbana::BufferIBV indices;
  higanbana::ShaderArguments args;
  higanbana::ShaderArguments meshArgs;
  higanbana::BufferSRV vertices;
  higanbana::BufferSRV uvs;
  higanbana::BufferSRV normals;
  // mesh shader required
  higanbana::BufferSRV uniqueIndices;
  higanbana::BufferSRV packedIndices;
  higanbana::BufferSRV meshlets;
};

class MeshSystem
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<MeshViews> views;

public:
  int allocate(higanbana::GpuGroup& gpu, higanbana::ShaderArgumentsLayout& normalLayout,higanbana::ShaderArgumentsLayout& meshLayout, MeshData& data);
  MeshViews& operator[](int index) { return views[index]; }
  void free(int index);
};
}