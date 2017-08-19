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
          .setDepthEnable(false));

      pipeline = device.createGraphicsPipeline(pipelineDescriptor);
      renderpass = device.createRenderpass();
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
      //binding.srv(::shader::ImGui::tex)
      for (int i = 0; i < drawData->CmdListsCount; ++i)
      {
        auto list = drawData->CmdLists[i];
        auto vbv = device.dynamicBuffer(makeByteView(list->VtxBuffer.Data, list->VtxBuffer.size() * sizeof(list->VtxBuffer[0])));
        auto ibv = device.dynamicBuffer(makeByteView(list->IdxBuffer.Data, list->IdxBuffer.size() * sizeof(list->IdxBuffer[0])));

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
            
            //setScissor({ int2(clipRect.x, clipRect.y), int2(clipRect.z, clipRect.w) });
            binding.srv(::shader::ImGui::vertices, vbv);
            
            //setShaderView(ImguiRenderer::tex, imgui.fontAtlas);
            //drawIndexed(d.ElemCount, indexOffset);
          }

          indexOffset += d.ElemCount;
        }
      }
    }
  }
}