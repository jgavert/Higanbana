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
      enum class FitMode
      {
        Fill,
        Fit,
        Stretch
      };

      Blitter(GpuDevice& device);

      void blit(GpuDevice& device, CommandGraph& graph, TextureRTV& target, TextureSRV& source, float2 topleft, float2 size);
      void blit(GpuDevice& device, CommandGraph& graph, TextureRTV& target, TextureSRV& source, int2 topleft, int2 size);
      void blitImage(GpuDevice& device, CommandGraph& graph, TextureRTV& target, TextureSRV& source, FitMode mode);
    };
  }
}