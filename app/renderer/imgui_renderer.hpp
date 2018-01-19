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
      Texture fontatlas;
      TextureSRV fontatlasSrv;
    public:
      ImGui(GpuDevice& device);
      void beginFrame(TextureRTV& target);
      void endFrame(GpuDevice& device, CommandGraphNode& graph, TextureRTV& target);
    };
  }
}