#pragma once
#include "faze/src/new_gfx/GraphicsCore.hpp"

namespace faze
{
  namespace renderer
  {
    class Postprocess 
    {
      ComputePipeline pipeline;
    public:
      Postprocess(GpuDevice& device);

      void postprocess(CommandGraphNode& graph, TextureUAV& target, TextureSRV& source, TextureSRV& luminance);
    };
  }
}