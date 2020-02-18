#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>
#include "../world/visual_data_structures.hpp"

namespace app
{
struct MaterialViews
{
  higanbana::TextureSRV albedo;
};

class MaterialDB
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<MaterialViews> views;

public:
  int allocate(higanbana::GpuGroup& gpu, higanbana::ShaderArgumentsLayout& materialLayout, MaterialData& data);
  MaterialViews& operator[](int index) { return views[index]; }
  void free(int index);
};
}