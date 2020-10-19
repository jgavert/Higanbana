#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/graphics/helpers/multi_pipeline.hpp>

namespace app::renderer
{
class Blitter
{
  higanbana::ShaderArgumentsLayout m_input;
  higanbana::GraphicsPipelines<higanbana::FormatType> pipelines;
  higanbana::Renderpasses<higanbana::FormatType> renderpasses;
public:
  enum class FitMode
  {
    Fill,
    Fit,
    Stretch
  };

  Blitter(higanbana::GpuGroup& device);

  void beginRenderpass(higanbana::CommandGraphNode& node, higanbana::TextureRTV target);
  void blit(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV target, higanbana::TextureSRV& source, float2 topleft, float2 size);
  void blit(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV target, higanbana::TextureSRV& source, int2 topleft, int2 size);
  void blitScale(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV target, higanbana::TextureSRV& source, int2 itopleft, float scale);
  void blitImage(higanbana::GpuGroup& device, higanbana::CommandGraphNode& node, higanbana::TextureRTV target, higanbana::TextureSRV& source, FitMode mode);
};
}