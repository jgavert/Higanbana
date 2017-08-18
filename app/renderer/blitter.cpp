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

    void Blitter::blit(GpuDevice& device, CommandGraph& graph, TextureRTV& target, TextureSRV& source, int2 itopleft, int2 isize)
    {
      float x = float(isize.x()) / float(target.desc().desc.width);
      float y = float(isize.y()) / float(target.desc().desc.height);

      float2 size = { x, y };

      x = float(itopleft.x()) / float(target.desc().desc.width);
      y = float(itopleft.y()) / float(target.desc().desc.height);

      float2 topleft = { x, y };

      vector<float4> vertices;
      float left = topleft.x() * 2.f - 1.f;
      float top = 1.f - (topleft.y() * 2.f);
      float right = left + size.x()*2.f; // -0.8f + 1.f
      float bottom = top - size.y()*2.f; // 0.8f - 1.f

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

    void Blitter::blitImage(GpuDevice& device, CommandGraph& graph, TextureRTV& target, TextureSRV& source, FitMode mode)
    {
      float dstScale = float(target.desc().desc.width) / float(target.desc().desc.height);
      float srcScale = float(source.desc().desc.width) / float(source.desc().desc.height);
      //float scaleDiff = dstScale - srcScale;

      float x = 2.f;
      float y = 2.f;

      float offsetX = 0.f;
      float offsetY = 0.f;
      if (mode == FitMode::Fit)
      {
        if (dstScale > srcScale)
        {
          x *= 1.f - (dstScale - srcScale) / dstScale;
          offsetX = (2.f - x) / 2.f;
        }
        else
        {
          y *= 1.f - (srcScale - dstScale) / srcScale;
          offsetY = (2.f - y) / 2.f;
        }
      }
      else if (mode == FitMode::Fill)
      {
        if (dstScale < srcScale)
        {
          x *= 1.f - (dstScale - srcScale) / dstScale;
          offsetX = (2.f - x) / 2.f;
        }
        else
        {
          y *= 1.f - (srcScale - dstScale) / srcScale;
          offsetY = (2.f - y) / 2.f;
        }
      }

      vector<float4> vertices;
      float left = offsetX - 1.f;
      float top = 1.f - offsetY;
      float right = left + x; // -0.8f + 1.f
      float bottom = top - y; // 0.8f - 1.f

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