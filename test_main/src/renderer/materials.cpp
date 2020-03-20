#include "materials.hpp"

namespace app
{
int MaterialDB::allocate(higanbana::GpuGroup& gpu, MaterialData& data) {
  auto val = freelist.allocate();
  if (views.size() < val+1) views.resize(val+1);
  views[val].data = data;
  views[val].dirty = true;
  return val;
}
void MaterialDB::update(higanbana::GpuGroup& gpu, higanbana::CommandGraphNode& node) {
  using namespace higanbana;
  bool copyAll = false;
  if (m_materials.handle().id == higanbana::ResourceHandle::InvalidId || m_materials.desc().desc.width != views.size())
  {
    m_materials = gpu.createBuffer(ResourceDescriptor()
      .setCount(views.size())
      .setStructured<MaterialData>()
      .setName("materials")
      .setUsage(ResourceUsage::GpuReadOnly));
    m_materialsSRV = gpu.createBufferSRV(m_materials);
    copyAll = true;
  }
  int index = 0;
  for (auto&& it : views) {
    if (it.dirty || copyAll) {
      auto materialData = gpu.dynamicBuffer<MaterialData>(it.data);
      node.copy(m_materials, index, materialData);
      //it.dirty = false;
    }
    index++;
  }
}

void MaterialDB::free(int index) {
  freelist.release(index);
}
}