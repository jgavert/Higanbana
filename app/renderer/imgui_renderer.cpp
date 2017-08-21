#include "imgui_renderer.hpp"

#include "app/shaders/imgui.if.hpp"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "core/src/external/imgiu/imgui.h"

namespace faze
{
  namespace renderer
  {
    ImGui::ImGui(GpuDevice& device)
    {
		auto pipelineDescriptor = GraphicsPipelineDescriptor()
			.setVertexShader("imgui")
			.setPixelShader("imgui")
			.setPrimitiveTopology(PrimitiveTopology::Triangle)
			.setDepthStencil(DepthStencilDescriptor()
				.setDepthEnable(false))
			
			.setRasterizer(RasterizerDescriptor()
				.setCullMode(CullMode::None)
				.setFillMode(FillMode::Solid))
				
														
			.setBlend(BlendDescriptor()
				.setRenderTarget(0, RTBlendDescriptor()
					.setBlendEnable(true)
					.setSrcBlend(Blend::SrcAlpha)
					.setDestBlend(Blend::InvSrcAlpha)
					.setBlendOp(BlendOp::Add)
					.setSrcBlendAlpha(Blend::SrcAlpha)
					.setDestBlendAlpha(Blend::InvSrcAlpha)
					.setBlendOpAlpha(BlendOp::Add)))  
			;							   

      pipeline = device.createGraphicsPipeline(pipelineDescriptor);
      renderpass = device.createRenderpass();

	  uint8_t *pixels = nullptr;
	  int x, y;
	  auto &io = ::ImGui::GetIO();
	  io.DeltaTime = 1.f / 60.f;
	  io.Fonts->AddFontDefault();
	  io.Fonts->GetTexDataAsAlpha8(&pixels, &x, &y);

	  CpuImage image(ResourceDescriptor()
		  .setSize({ x, y, 1 })
		  .setFormat(FormatType::Unorm8)
		  .setDimension(FormatDimension::Texture2D)
		  .setName("ImGui Font Atlas")
		  .setUsage(ResourceUsage::GpuReadOnly));
	  auto sr = image.subresource(0, 0);
	  memcpy(sr.data(), pixels, sr.size());

	  fontatlas = device.createTexture(image);
	  fontatlasSrv = device.createTextureSRV(fontatlas);
    }

    void ImGui::beginFrame(TextureRTV& target)
    {
      ImGuiIO &io = ::ImGui::GetIO();
      io.DisplaySize = { float(target.texture().desc().desc.width), float(target.texture().desc().desc.height) };
      io.DeltaTime = (float)(1.0f / 60.0f);

      ::ImGui::NewFrame();
    }

    void ImGui::endFrame(GpuDevice& device, CommandGraph& graph, TextureRTV& target)
    {
      ::ImGui::Render();
      auto drawData = ::ImGui::GetDrawData();
      F_ASSERT(drawData->Valid, "ImGui draw data is invalid!");

      auto& node = graph.createPass2("ImGui");
      node.renderpass(renderpass);
      node.subpass(target);

      auto binding = node.bind<::shader::ImGui>(pipeline);

      binding.constants.reciprocalResolution = float2{ 1.f, 1.f } / float2{ float(target.texture().desc().desc.width), float(target.texture().desc().desc.height) };
	  binding.srv(::shader::ImGui::tex, fontatlasSrv);

      for (int i = 0; i < drawData->CmdListsCount; ++i)
      {
        auto list = drawData->CmdLists[i];
		// 5x 32bit thing...
        auto vbv = device.dynamicBuffer(makeByteView(list->VtxBuffer.Data, list->VtxBuffer.size() * sizeof(list->VtxBuffer[0])), FormatType::Uint32);
        auto ibv = device.dynamicBuffer(makeByteView(list->IdxBuffer.Data, list->IdxBuffer.size() * sizeof(list->IdxBuffer[0])), FormatType::Uint16);

		binding.srv(::shader::ImGui::vertices, vbv);

        uint indexOffset = 0;
        for (auto &d : list->CmdBuffer)
        {
          if (d.UserCallback)
          {
            d.UserCallback(list, &d);
          }
          else
          {
            auto clipRect = d.ClipRect;
			node.setScissor(int2{static_cast<int>(clipRect.x), static_cast<int>(clipRect.y)}, int2{ static_cast<int>(clipRect.z), static_cast<int>(clipRect.w)});
            node.drawIndexed(binding, ibv, d.ElemCount, 1, indexOffset);
          }

          indexOffset += d.ElemCount;
        }
      }
    }
  }
}