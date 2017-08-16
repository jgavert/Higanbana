#include "blitter.hpp"

#include "app/shaders/blitter.if.hpp"

namespace faze
{
  namespace renderer
  {
    Blitter::Blitter(GpuDevice& device)
    {
      auto pipelineDescriptor = GraphicsPipelineDescriptor()
        .setVertexShader("blitter")
        .setPixelShader("blitter")
        .setPrimitiveTopology(PrimitiveTopology::Triangle)
        .setDepthStencil(DepthStencilDescriptor()
          .setDepthEnable(false));

      pipeline = device.createGraphicsPipeline(pipelineDescriptor);
      renderpass = device.createRenderpass();
    }

    void Blitter::blit(GpuDevice& device, CommandGraph& graph, TextureRTV& target, TextureSRV& source, float2 topleft, float2 size)
    {
      vector<float4> vertices;

      /*
      Draw quad

              ------
              |   /|
              |  / |
              | /  |
              |/___|

      like so.
      */

      float left = topleft.x();
      float top = topleft.y();
      float right = topleft.x() + size.x(); // -0.8f + 1.f
      float bottom = topleft.y() - size.y(); // 0.8f - 1.f

      vertices.push_back(float4{ left, bottom,  0.f, 0.f });
      vertices.push_back(float4{ left,  top,    0.f, 1.f });
      vertices.push_back(float4{ right, top,    1.f, 1.f });

      vertices.push_back(float4{ right, bottom, 1.f, 0.f });
      vertices.push_back(float4{ left,  bottom, 0.f, 0.f });
      vertices.push_back(float4{ right, top,    1.f, 1.f });

      auto verts = device.dynamicBuffer(makeMemView(vertices), FormatType::Float32x4);

      auto rp = graph.createPass("Blit");
      rp.renderpass(renderpass);
      rp.subpass(target);
      auto binding = rp.bind<::shader::Blitter>(pipeline);
      binding.srv(::shader::Blitter::vertices, verts);
      binding.srv(::shader::Blitter::source, source);
      rp.draw(binding, 6, 1);
      rp.endRenderpass();
      
      graph.addPass(std::move(rp));
    }
  }
}