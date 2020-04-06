#include "imgui.hpp"

using namespace higanbana;

SHADER_STRUCT(imguiConstants,
  float2 reciprocalResolution;
  uint renderCustom;
);

namespace app::renderer 
{
IMGui::IMGui(GpuGroup& device)
{
  ShaderArgumentsLayoutDescriptor argsLayoutDesc = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::ByteAddressBuffer, "vertices");
  ShaderArgumentsLayoutDescriptor argsLayoutDesc2 = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Texture2D, "float", "tex")
    .readOnlyBindless(ShaderResourceType::Texture2D, "viewports", 20);
  vertexLayout = device.createShaderArgumentsLayout(argsLayoutDesc);
  imageLayout = device.createShaderArgumentsLayout(argsLayoutDesc2);

  higanbana::PipelineInterfaceDescriptor imguiInterface = PipelineInterfaceDescriptor()
    .constants<imguiConstants>()
    .shaderArguments(0, imageLayout)
    .shaderArguments(1, vertexLayout);

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setInterface(imguiInterface)
    .setVertexShader("imgui")
    .setPixelShader("imgui")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false))

    .setRasterizer(RasterizerDescriptor()
      .setCullMode(CullMode::None)
      .setFillMode(FillMode::Solid))
    .setRenderTargetCount(1)
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

  image = CpuImage(ResourceDescriptor()
    .setSize(int2{ x, y })
    .setFormat(FormatType::Unorm8)
    .setName("ImGui Font Atlas")
    .setUsage(ResourceUsage::GpuReadOnly));
  auto sr = image.subresource(0, 0);
  memcpy(sr.data(), pixels, sr.size());

  fontatlas = device.createTexture(image);
  fontatlasSrv = device.createTextureSRV(fontatlas);

#if defined(FAZE_PLATFORM_WINDOWS)
  io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
  io.KeyMap[ImGuiKey_Home] = VK_HOME;
  io.KeyMap[ImGuiKey_End] = VK_END;
  io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
#endif
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';
}

void IMGui::render(GpuGroup& device, CommandGraphNode& node, TextureRTV& target, vector<TextureSRV>& viewports)
{
  auto drawData = ::ImGui::GetDrawData();
  HIGAN_ASSERT(drawData->Valid, "ImGui draw data is invalid!");
  target.setOp(higanbana::LoadOp::Load);
  target.setOp(higanbana::StoreOp::Store);
  node.renderpass(renderpass, target);

  auto binding = node.bind(pipeline);

  imguiConstants constants;
  constants.reciprocalResolution = math::div(float2{ 1.f, 1.f }, float2{ float(target.texture().desc().desc.width), float(target.texture().desc().desc.height) });
  constants.renderCustom = 0;
  auto bindArgs = ShaderArgumentsDescriptor("IMGui vertices", vertexLayout);
  auto alternateArgs = ShaderArgumentsDescriptor("IMGui custom Image", imageLayout);
  alternateArgs.bind("tex", fontatlasSrv);
  alternateArgs.bindBindless("viewports", viewports);
  auto customImageArgs = device.createShaderArguments(alternateArgs);
  binding.arguments(0, customImageArgs);
  binding.constants(constants);

  for (int i = 0; i < drawData->CmdListsCount; ++i)
  {
    auto list = drawData->CmdLists[i];
    // 5x 32bit thing...
    auto vbv = device.dynamicBuffer(makeByteView(list->VtxBuffer.Data, list->VtxBuffer.size_in_bytes()), FormatType::Raw32);
    auto ibv = device.dynamicBuffer(makeByteView(list->IdxBuffer.Data, list->IdxBuffer.size_in_bytes()), FormatType::Uint16);

    bindArgs.bind("vertices", vbv);
    auto args = device.createShaderArguments(bindArgs);
    binding.arguments(1, args);

    uint indexOffset = 0;
    for (auto &d : list->CmdBuffer)
    {
      if (d.UserCallback)
      {
        d.UserCallback(list, &d);
      }
      else
      {
        if (d.TextureId != nullptr) {
          constants.renderCustom = reinterpret_cast<int>(d.TextureId);
          binding.constants(constants);
        }
        auto clipRect = d.ClipRect;
        node.setScissor(int2{ static_cast<int>(clipRect.x), static_cast<int>(clipRect.y) }, int2{ static_cast<int>(clipRect.z), static_cast<int>(clipRect.w) });
        node.drawIndexed(binding, ibv, d.ElemCount, 1, indexOffset);
        if (d.TextureId != nullptr) {
          constants.renderCustom = 0;
          binding.constants(constants);
        }
      }

      indexOffset += d.ElemCount;
    }
  }
  node.endRenderpass();
}
}