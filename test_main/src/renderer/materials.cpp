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
}

void MaterialDB::free(int index) {
  freelist.release(index);
}
}