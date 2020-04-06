#include "textures.hpp"
#include "../world/visual_data_structures.hpp"

namespace app
{
TextureDB::TextureDB(higanbana::GpuGroup& gpu) {
  using namespace higanbana;
  m_bindless = gpu.createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor()
    .readOnly<MaterialData>(ShaderResourceType::StructuredBuffer, "materials")
    .readOnlyBindless(ShaderResourceType::Texture2D, "materialTextures", 190));
}
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
  freelist.release(index);
  views[index] = {};
}

higanbana::ShaderArguments TextureDB::bindlessArgs(higanbana::GpuGroup& gpu, higanbana::BufferSRV materials) {
  using namespace higanbana;
  auto desc = ShaderArgumentsDescriptor("materials", m_bindless);
  desc.bind("materials", materials);
  desc.bindBindless("materialTextures", views);
  m_bindlessSet = gpu.createShaderArguments(desc);
  return m_bindlessSet;
}
}