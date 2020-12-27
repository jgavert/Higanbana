#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/platform/Window.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/time.hpp>
#include <higanbana/graphics/helpers/pingpongTexture.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>
#include <higanbana/core/system/LBS.hpp>
#include <css/task.hpp>
#include "renderer/imgui.hpp"
#include "renderer/world_renderer.hpp"
#include "renderer/mesh_test.hpp"
#include "renderer/tsaa.hpp"
#include "renderer/tonemapper.hpp"
#include "renderer/blitter.hpp"
#include "renderer/cubes.hpp"
#include "renderer/blocks.hpp"
#include "renderer/meshes.hpp"
#include "renderer/textures.hpp"
#include "renderer/materials.hpp"
#include "renderer/generate_image.hpp"
#include "renderer/generate_mips.hpp"
#include "renderer/depth_pyramid.hpp"
#include "renderer/particles.hpp"
#include "renderer/viewport.hpp"
#include "world/visual_data_structures.hpp"


namespace app
{
struct RendererOptions
{
  bool submitExperimental = true;
  bool submitSingleThread = false;
  bool submitLBS = false;
  bool particles = false;
  bool particlesSimulate = true;
  bool allowMeshShaders = false;
  bool allowRaytracing = false;
  bool unbalancedCubes = false;
  bool renderImGui = true;
  bool testMipper = false;
  bool renderCpuRaytrace = true;
  bool renderBlocks = true;
  bool enableHDR = false;
  void drawImGuiOptions()
  {
    ImGui::Checkbox("render blocks", &renderBlocks);
    ImGui::Checkbox("test generate mipmaps", &testMipper);
    ImGui::Checkbox("cpu raytracer", &renderCpuRaytrace);
    ImGui::Checkbox("HDR", &enableHDR);
    ImGui::Checkbox("Submit with experimental mt", &submitExperimental);
    ImGui::Checkbox("Submit Singlethread", &submitSingleThread);
    ImGui::Checkbox("Submit using CSS", &submitLBS);
    ImGui::Checkbox("Particles Enabled", &particles);
    ImGui::Checkbox("Simulate Particles", &particlesSimulate);

    //ImGui::Checkbox("Allow Mesh Shaders", &allowMeshShaders);
    //ImGui::Checkbox("Allow Raytracing", &allowRaytracing);
    ImGui::Checkbox("Unbalanced cube drawlists", &unbalancedCubes);
  }
};
struct ViewportOptions
{
  bool visible = false;
  int gpuToUse = 0;
  bool useMeshShaders = false;
  bool useRaytracing = true;
  bool syncResolutionToSwapchain = false;
  bool jitterEnabled = true;
  bool particlesDraw = true;
  bool tsaa = true;
  bool tsaaDebug = false;
  bool debugTextures = false;
  int tilesToComputePerFrame = 4;
  bool raytraceRealtime = false;
  //int tileSize = 32;
  int samplesPerPixel = 1;
  float resolutionScale = 1.f;
  bool writeStraightToBackbuffer = false;

  void ToggleButton(const char* hidden_id, bool* v)
  {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    ImU32 falseColor = IM_COL32(210, 50, 50, 255);
    ImU32 trueColor = IM_COL32(50, 210, 50, 255);
    ImU32 falseHovColor = IM_COL32(210, 50+20, 50+20, 255);
    ImU32 trueHovColor = IM_COL32(50+20, 210, 50+20, 255);

    if (ImGui::InvisibleButton(hidden_id, ImVec2(width, height)))
        *v = !*v;
    ImU32 col_bg;
    if (ImGui::IsItemHovered())
        col_bg = *v ? trueHovColor : falseHovColor;
    else
        col_bg = *v ? trueColor : falseColor;

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
    draw_list->AddCircleFilled(ImVec2(*v ? (p.x + width - radius) : (p.x + radius), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
  }

  void drawImGuiOptions(higanbana::vector<higanbana::GpuInfo>& gpus)
  {
    bool isInteropDevice = false;
    if (gpus.size() == 2 && gpus[0].api != gpus[1].api)
      isInteropDevice = true;

    if (isInteropDevice) {
      bool current = gpus[gpuToUse].api == higanbana::GraphicsApi::DX12;
      bool before = current;
      ImGui::Text("Vulkan"); ImGui::SameLine();
      ToggleButton("##toggle", &current); ImGui::SameLine();
      ImGui::Text("DX12");
      if (current != before) {
        gpuToUse = gpuToUse == 1 ? 0 : 1;
      }
    } else if (gpus.size() > 1)
      ImGui::SliderInt("GPU Device index: ", &gpuToUse, 0, static_cast<int>(gpus.size())-1);
    ImGui::Checkbox("sync resolution to viewport size", &syncResolutionToSwapchain);
    if (!syncResolutionToSwapchain)
    {
      ImGui::DragFloat("resolution scale", &resolutionScale, 0.01f, 0.0001f, 4.f);
    }
    else
    {
      resolutionScale = 1.f;
    }
    ImGui::Checkbox("debug textures", &debugTextures);
    ImGui::Checkbox("jitter viewport", &jitterEnabled);
    ImGui::Checkbox("TSAA", &tsaa);
    if (tsaa)
    {
      ImGui::Checkbox("  enable taa debugoutput", &tsaaDebug);
    }
    ImGui::Checkbox("Draw particles", &particlesDraw);
    ImGui::Checkbox("Mesh Shaders", &useMeshShaders);
    ImGui::Checkbox("Raytracing", &useRaytracing);
    if (useRaytracing) {
      ImGui::Checkbox("realtime", &raytraceRealtime);
      ImGui::DragInt("Tiles per frame", &tilesToComputePerFrame, 1, 1, 10000);
      //ImGui::DragInt("Tile size", &tileSize, 1, 2, 256);
      ImGui::DragInt("Samples per pixel", &samplesPerPixel, 1, 1, 1000);
    }
  }
};

struct RenderViewportInfo {
  int viewportIndex; // gained when allocating a viewport from renderer
  int2 viewportSize;
  ViewportOptions options;
  ActiveCamera camera;
  bool screenshot;
};

struct ReadbackTexture {
  std::string filepath;
  higanbana::ResourceDescriptor tex;
  higanbana::ReadbackFuture readback;
};

class Renderer
{
  higanbana::GraphicsSubsystem& graphics;
  higanbana::GpuGroup& dev;
  higanbana::GraphicsSurface surface;

  higanbana::SwapchainDescriptor scdesc;
  higanbana::Swapchain swapchain;

  // api logos
  higanbana::Texture m_logoDx12;
  higanbana::TextureSRV m_logoDx12Srv;
  higanbana::Texture m_logoVulkan;
  higanbana::TextureSRV m_logoVulkanSrv;

  // global layouts
  higanbana::ShaderArgumentsLayout m_camerasLayout;
  higanbana::Buffer cameras;
  higanbana::BufferSRV cameraSRV;
  higanbana::BufferUAV cameraUAV;
  higanbana::ShaderArguments cameraArgs;

  // meshes
  MeshSystem meshes;

  // materials
  TextureDB textures;
  MaterialDB materials;

  // renderers
  renderer::Cubes cubes;
  renderer::IMGui imgui;
  renderer::World worldRend;
  renderer::MeshTest worldMeshRend;
  renderer::Blocks blockRend;
  // postprocess
  renderer::ShittyTSAA tsaa;
  renderer::Tonemapper tonemapper;
  renderer::Blitter blitter;
  renderer::GenerateMips mipper;
  renderer::DepthPyramid depthPyramid;
  renderer::GenerateImage genImage;
  renderer::Particles particleSimulation;

  // resources
  higanbana::vector<app::Viewport> viewports;
  higanbana::PingPongTexture proxyTex; // used for background color

  // could probably be replaced with blitter...
  higanbana::ShaderArgumentsLayout blitLayout;
  higanbana::GraphicsPipeline composite;
  higanbana::Renderpass compositeRP;

  // temp, probably shouldnt be here
  //CameraSettings m_previousCamera;

  // readbacks
  higanbana::deque<ReadbackTexture> readbacks;

  // making submits more smooth
  std::unique_ptr<css::Task<void>> presentTask; // stores last present call

  // info
  std::optional<higanbana::SubmitTiming> m_previousInfo;
  int64_t m_acquireTimeTaken;
  void renderMeshes(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth, higanbana::ShaderArguments materials, int cameraIndex, int prevCamera, higanbana::vector<InstanceDraw>& instances);
  void renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, higanbana::ShaderArguments materials, int cameraIndex, higanbana::vector<InstanceDraw>& instances);

  void testMipper(higanbana::CommandNodeVector& tasks, higanbana::Texture testTexture, higanbana::TextureRTV& backbuffer, higanbana::Texture someData);

  struct SceneArguments {
    higanbana::TextureRTV gbufferRTV;
    higanbana::TextureDSV depth;
    higanbana::TextureRTV motionVectors;
    higanbana::ShaderArguments materials;
    ViewportOptions options;
    int cameraIdx;
    int prevCameraIdx;
    float4x4 perspective;
    float3 cameraPos;
    int drawcalls;
    int drawsSplitInto;
  };
  css::Task<void> renderScene(higanbana::CommandNodeVector& tasks, higanbana::WTime& time, const RendererOptions& rendererOptions, const SceneArguments args, higanbana::vector<InstanceDraw>& instances, higanbana::vector<ChunkBlockDraw>& blocks);
public:
  Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev);
  void initWindow(higanbana::Window& window, higanbana::GpuInfo info);
  void loadLogos(higanbana::FileSystem& fs);
  int2 windowSize();
  void ensureViewportCount(int size);
  void resizeExternal(higanbana::ResourceDescriptor& desc);
  //void render(higanbana::LBS& lbs, higanbana::WTime& time, RendererOptions options, ActiveCamera viewMat, higanbana::vector<InstanceDraw>& instances, int cubeCount, int cubeCommandLists, std::optional<higanbana::CpuImage>& heightmap);
  css::Task<void> renderViewports(higanbana::LBS& lbs, higanbana::WTime time, const RendererOptions options, higanbana::MemView<RenderViewportInfo> viewportsToRender, higanbana::vector<InstanceDraw>& instances, higanbana::vector<ChunkBlockDraw>& blocks, int cubeCount, int cubeCommandLists);
  std::optional<higanbana::SubmitTiming> timings();
  float acquireTimeTakenMs() {return float(m_acquireTimeTaken) / 1000000.f;}
  int loadMesh(MeshData& data, int buffer[5]);
  void unloadMesh(int index);
  int loadBuffer(BufferData& data);
  void unloadBuffer(int index);
  int loadTexture(TextureData& data);
  void unloadTexture(int index);
  int loadMaterial(MaterialData& data);
  void unloadMaterial(int index);

  void handleReadbacks(higanbana::FileSystem& fs);
  css::Task<void> cleanup();
};
}