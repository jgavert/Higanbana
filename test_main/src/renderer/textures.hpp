#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>

namespace app
{
class TextureDB 
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<higanbana::TextureSRV> views;

  higanbana::ShaderArgumentsLayout m_bindless;
  higanbana::ShaderArguments m_bindlessSet;

public:
  TextureDB(higanbana::GpuGroup& gpu);
  int allocate(higanbana::GpuGroup& gpu, higanbana::CpuImage& data);
  void free(int index);
  higanbana::TextureSRV& operator[](int index) { return views[index]; }
  higanbana::ShaderArgumentsLayout bindlessLayout() { return m_bindless; }
  higanbana::ShaderArguments bindlessArgs(higanbana::GpuGroup& gpu, higanbana::BufferSRV materials);
};
}