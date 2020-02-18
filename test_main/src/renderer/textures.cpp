#include "textures.hpp"

namespace app
{
  int TextureDB::allocate(higanbana::GpuGroup& gpu, higanbana::CpuImage& image)
  {
    auto val = freelist.allocate();
    if (views.size() < val+1) views.resize(val+1);
    auto& view = views[val];
    view = gpu.createTextureSRV(gpu.createTexture(image));
    return val;
  }

  void TextureDB::free(int index)
  {
    views[index] = {};
  }
}