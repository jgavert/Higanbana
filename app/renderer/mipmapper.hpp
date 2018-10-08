#pragma once
#include "faze/src/new_gfx/GraphicsCore.hpp"

namespace faze
{
  namespace renderer
  {
    class Mipmapper 
    {
      ComputePipeline pipeline;
    public:
      Mipmapper(GpuDevice& device);

      void generateMip(CommandGraphNode& graph, TextureUAV& target, TextureSRV& source, float2 texelSize, int2 dim);

      void generateMipLevels(GpuDevice& device, CommandGraphNode& graph, Texture& source, int startMip = 0);
      void generateMipLevelsTo(GpuDevice& device, CommandGraphNode& graph, Texture& target, Texture& source, int startMip = 0);
    };
  }
}