#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <imgui.h>

namespace app
{
  namespace renderer
  {
    class IMGui
    {
      higanbana::ShaderArgumentsLayout argsLayout;
      higanbana::GraphicsPipeline pipeline;
      higanbana::Renderpass renderpass;
      higanbana::Texture fontatlas;
      higanbana::TextureSRV fontatlasSrv;

      higanbana::CpuImage image;
    public:
      IMGui(higanbana::GpuGroup& device);
      void render(higanbana::GpuGroup& device, higanbana::CommandGraphNode& graph, higanbana::TextureRTV& target);
    };
  }
}