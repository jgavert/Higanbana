#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>
#include <higanbana/core/system/heap_allocator.hpp>
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
  higanbana::BufferSRV tangents;
  // mesh shader required
  higanbana::BufferSRV uniqueIndices;
  higanbana::BufferSRV packedIndices;
  higanbana::BufferSRV meshlets;
};

class MeshSystem
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<MeshViews> views;
  higanbana::FreelistAllocator freelistBuffers;
  higanbana::HeapAllocator meshbufferAllocator;
  higanbana::vector<higanbana::RangeBlock> sourceBuffers;
  higanbana::Buffer meshbuffer;

public:
  int allocate(higanbana::GpuGroup& gpu, higanbana::ShaderArgumentsLayout& normalLayout,higanbana::ShaderArgumentsLayout& meshLayout, MeshData& data, int buffers[5]);
  MeshViews& operator[](int index) { return views[index]; }
  void free(int index);
  int allocateBuffer(higanbana::GpuGroup& gpu, BufferData& data);
  void freeBuffer(int index);
};
}