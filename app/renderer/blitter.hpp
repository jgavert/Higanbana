#pragma once
#include "faze/src/new_gfx/GraphicsCore.hpp"

namespace faze
{
  namespace renderer
  {
    class Blitter
    {
      GraphicsPipeline pipeline;
      Renderpass renderpass;
    public:
      Blitter(GpuDevice& device);

      void blit(GpuDevice& device, CommandGraph& graph, TextureRTV& target, TextureSRV& source, float2 topleft, float2 size);
    };
  }
}