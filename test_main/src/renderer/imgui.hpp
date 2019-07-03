#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <imgui.h>

namespace app
{
  namespace renderer
  {
    class IMGui
    {
      higanbana::GraphicsPipeline pipeline;
      higanbana::Renderpass renderpass;
      higanbana::Texture fontatlas;
      higanbana::TextureSRV fontatlasSrv;
    public:
      IMGui(higanbana::GpuGroup& device);
      void beginFrame(higanbana::TextureRTV& target);
      void endFrame(higanbana::GpuGroup& device, higanbana::CommandGraphNode& graph, higanbana::TextureRTV& target);
    };
  }
}