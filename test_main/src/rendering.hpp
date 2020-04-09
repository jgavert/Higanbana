#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/platform/Window.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/time.hpp>
#include <higanbana/graphics/helpers/pingpongTexture.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>
#include <higanbana/core/system/LBS.hpp>
#include "renderer/imgui.hpp"
#include "renderer/world_renderer.hpp"
#include "renderer/mesh_test.hpp"
#include "renderer/tsaa.hpp"
#include "renderer/tonemapper.hpp"
#include "renderer/blitter.hpp"
#include "renderer/cubes.hpp"
#include "renderer/meshes.hpp"
#include "renderer/textures.hpp"
#include "renderer/materials.hpp"
#include "renderer/generate_image.hpp"
#include "renderer/particles.hpp"
#include "renderer/viewport.hpp"
#include "world/visual_data_structures.hpp"


namespace app
{
struct RendererOptions
{
  bool submitExperimental = false;
  bool submitSingleThread = false;
  bool submitLBS = false;
  bool particles = false;
  bool particlesSimulate = true;
  bool allowMeshShaders = false;
  bool allowRaytracing = false;
  bool unbalancedCubes = false;
  bool renderImGui = true;
  void drawImGuiOptions()
  {
    ImGui::Checkbox("Submit with experimental mt", &submitExperimental);
    ImGui::Checkbox("Submit Singlethread", &submitSingleThread);
    ImGui::Checkbox("Submit using LBS", &submitLBS);
    ImGui::Checkbox("Particles", &particles);
    ImGui::Checkbox("Simulate", &particlesSimulate);
    ImGui::Checkbox("Allow Mesh Shaders", &allowMeshShaders);
    ImGui::Checkbox("Allow Raytracing", &allowRaytracing);
    ImGui::Checkbox("Unbalanced cube drawlists", &unbalancedCubes);
  }
};
struct ViewportOptions
{
  bool visible = false;
  int gpuToUse = 0;
  bool useMeshShaders = false;
  bool useRaytracing = false;
  bool syncResolutionToSwapchain = false;
  bool jitterEnabled = true;
  bool particlesDraw = true;
  bool tsaa = true;
  bool tsaaDebug = false;
  bool debugTextures = false;
  float resolutionScale = 1.f;
  bool writeStraightToBackbuffer = false;

  void drawImGuiOptions(higanbana::vector<higanbana::GpuInfo>& gpus)
  {
    ImGui::SliderInt("GPU Device index: ", &gpuToUse, 0, static_cast<int>(gpus.size()));
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
  }
};

struct RenderViewportInfo {
  int viewportIndex; // gained when allocating a viewport from renderer
  int2 viewportSize;
  ViewportOptions options;
  ActiveCamera camera;
};

class Renderer
{
  higanbana::GraphicsSubsystem& graphics;
  higanbana::GpuGroup& dev;
  higanbana::GraphicsSurface surface;

  higanbana::SwapchainDescriptor scdesc;
  higanbana::Swapchain swapchain;

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
  // postprocess
  renderer::ShittyTSAA tsaa;
  renderer::Tonemapper tonemapper;
  renderer::Blitter blitter;
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

  // info
  std::optional<higanbana::SubmitTiming> m_previousInfo;
  int64_t m_acquireTimeTaken;
  void renderMeshes(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureRTV& motionVecs, higanbana::TextureDSV& depth, higanbana::ShaderArguments materials, int cameraIndex, int prevCamera, higanbana::vector<InstanceDraw>& instances);
  void renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, higanbana::ShaderArguments materials, int cameraIndex, higanbana::vector<InstanceDraw>& instances);

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
  void renderScene(higanbana::CommandNodeVector& tasks, higanbana::WTime& time, const RendererOptions& rendererOptions, const SceneArguments& args, higanbana::vector<InstanceDraw>& instances);
public:
  Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev);
  void initWindow(higanbana::Window& window, higanbana::GpuInfo info);
  int2 windowSize();
  void ensureViewportCount(int size);
  void resizeExternal(higanbana::ResourceDescriptor& desc);
  //void render(higanbana::LBS& lbs, higanbana::WTime& time, RendererOptions options, ActiveCamera viewMat, higanbana::vector<InstanceDraw>& instances, int cubeCount, int cubeCommandLists, std::optional<higanbana::CpuImage>& heightmap);
  void renderViewports(higanbana::LBS& lbs, higanbana::WTime& time, const RendererOptions& options, higanbana::MemView<RenderViewportInfo> viewportsToRender, higanbana::vector<InstanceDraw>& instances, int cubeCount, int cubeCommandLists);
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
};
}