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
#include "renderer/blitter.hpp"
#include "renderer/cubes.hpp"
#include "renderer/meshes.hpp"
#include "renderer/textures.hpp"
#include "renderer/materials.hpp"
#include "renderer/generate_image.hpp"
#include "renderer/particles.hpp"
#include "world/visual_data_structures.hpp"


namespace app
{
struct RendererOptions
{
  bool submitExperimental = false;
  bool submitSingleThread = false;
  bool submitLBS = false;
  bool unbalancedCubes = false;
  bool allowMeshShaders = false;
  bool allowRaytracing = false;
  bool syncResolutionToSwapchain = false;
  bool jitterEnabled = true;
  bool particles = false;
  bool particlesSimulate = true;
  bool particlesDraw = true;
  bool tsaa = true;
  bool tsaaDebug = false;
  bool debugTextures = false;
  float resolutionScale = 1.f;

  void drawImGuiOptions()
  {
    if (ImGui::CollapsingHeader("Renderer Options"))
    {
      ImGui::Checkbox("sync resolution to window size", &syncResolutionToSwapchain);
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
        ImGui::Checkbox("enable taa debugoutput", &tsaaDebug);
      }
      ImGui::Checkbox("Particles", &particles);
      if (particles)
      {
        ImGui::Checkbox(" Simulate", &particlesSimulate);
        ImGui::Checkbox(" Draw", &particlesDraw);
      }
      ImGui::Checkbox("allow Mesh Shaders", &allowMeshShaders); ImGui::SameLine();
      ImGui::Checkbox("allow Raytracing", &allowRaytracing);
      ImGui::Checkbox("submit multithread experimental", &submitExperimental);
      ImGui::Checkbox("submit singlethread", &submitSingleThread);
      ImGui::Checkbox("submit lbs", &submitLBS);
      ImGui::Checkbox("unbalanced cube drawlists", &unbalancedCubes);
    }
  }
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
  renderer::Blitter blitter;
  renderer::GenerateImage genImage;
  renderer::Particles particleSimulation;

  // resources
  // gbuffer??
  higanbana::Texture m_gbuffer;
  higanbana::TextureRTV m_gbufferRTV;
  higanbana::TextureSRV m_gbufferSRV;
  higanbana::Texture m_depth;
  higanbana::TextureDSV m_depthDSV;
  higanbana::Texture depthExternal;
  higanbana::TextureDSV depthExternalDSV;
  // other
  higanbana::PingPongTexture tsaaResolved; // tsaa history/current
  higanbana::Texture tsaaDebug;
  higanbana::TextureSRV tsaaDebugSRV;
  higanbana::TextureUAV tsaaDebugUAV;

  higanbana::PingPongTexture proxyTex; // used for background color

  // could probably be replaced with blitter...
  higanbana::ShaderArgumentsLayout blitLayout;
  higanbana::GraphicsPipeline composite;
  higanbana::Renderpass compositeRP;

  // info
  std::optional<higanbana::SubmitTiming> m_previousInfo;
  void renderMeshes(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer,higanbana::TextureDSV& depth,higanbana::ShaderArguments materials,int cameraIndex, higanbana::vector<InstanceDraw>& instances);
  void renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer,higanbana::TextureDSV& depth,higanbana::ShaderArguments materials,int cameraIndex, higanbana::vector<InstanceDraw>& instances);

  struct SceneArguments{
    higanbana::TextureRTV gbufferRTV;
    higanbana::TextureDSV depth;
    higanbana::ShaderArguments materials;
    RendererOptions options;
    int cameraIdx;
    float4x4 perspective;
    float3 cameraPos;
    int drawcalls;
    int drawsSplitInto;
  };
  void renderScene(higanbana::CommandGraph& tasks, higanbana::WTime& time, const SceneArguments& args, higanbana::vector<InstanceDraw>& instances, std::optional<higanbana::CpuImage>& heightmap);
public:
  Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev);
  void initWindow(higanbana::Window& window, higanbana::GpuInfo info);
  int2 windowSize();
  void resizeExternal(higanbana::ResourceDescriptor& desc);
  void resizeInternal(higanbana::ResourceDescriptor& desc);
  void render(higanbana::LBS& lbs, higanbana::WTime& time, RendererOptions options, ActiveCamera viewMat, higanbana::vector<InstanceDraw>& instances, int cubeCount, int cubeCommandLists, std::optional<higanbana::CpuImage>& heightmap);
  std::optional<higanbana::SubmitTiming> timings();
  int loadMesh(MeshData& data);
  void unloadMesh(int index);
  int loadTexture(TextureData& data);
  void unloadTexture(int index);
  int loadMaterial(MaterialData& data);
  void unloadMaterial(int index);
};
}