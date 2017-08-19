#pragma once
#include "faze/src/new_gfx/GraphicsCore.hpp"

namespace faze
{
  namespace renderer
  {
    class ImGui 
    {
      GraphicsPipeline pipeline;
      Renderpass renderpass;
    public:
      ImGui(GpuDevice& device);
      void beginFrame(TextureRTV& target);
      void endFrame(GpuDevice& device, CommandGraph& graph, TextureRTV& target);
    };
  }
}