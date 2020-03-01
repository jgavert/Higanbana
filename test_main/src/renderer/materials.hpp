#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>
#include "../world/visual_data_structures.hpp"

namespace app
{
struct MaterialViews
{
  bool dirty;
  MaterialData data;
};

class MaterialDB
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<MaterialViews> views;
  higanbana::Buffer m_materials;
  higanbana::BufferSRV m_materialsSRV;
  higanbana::BufferUAV m_materialsUAV;


public:
  int allocate(higanbana::GpuGroup& gpu, MaterialData& data);
  void free(int index);
  MaterialData& updateMaterial(int index){
    views[index].dirty = true;
    return views[index].data;
  }
  MaterialViews& operator[](int index) { return views[index]; }
  void update(higanbana::GpuGroup& gpu, higanbana::CommandGraphNode& node);
  higanbana::BufferSRV srv() { return m_materialsSRV; }
};
}