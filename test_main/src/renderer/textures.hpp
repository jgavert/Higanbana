#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>

namespace app
{
class TextureDB 
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<higanbana::TextureSRV> views;

public:
  int allocate(higanbana::GpuGroup& gpu, higanbana::CpuImage& data);
  higanbana::TextureSRV& operator[](int index) { return views[index]; }
  void free(int index);
};
}