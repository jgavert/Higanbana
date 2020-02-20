#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/platform/Window.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/time.hpp>
#include <higanbana/graphics/helpers/pingpongTexture.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>
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
  bool allowMeshShaders = false;
  bool allowRaytracing = false;
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
  higanbana::Texture gbuffer;
  higanbana::TextureRTV gbufferRTV;
  higanbana::TextureSRV gbufferSRV;
  higanbana::Texture depth;
  higanbana::TextureDSV depthDSV;
  // other
  higanbana::PingPongTexture tsaaResolved; // tsaa history/current

  higanbana::PingPongTexture proxyTex; // used for background color

  // could probably be replaced with blitter...
  higanbana::ShaderArgumentsLayout blitLayout;
  higanbana::GraphicsPipeline composite;
  higanbana::Renderpass compositeRP;

  // info
  higanbana::WTime time;
  std::optional<higanbana::SubmitTiming> m_previousInfo;
  void renderMeshes(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances);
  void renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances);

public:
  Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev);
  void initWindow(higanbana::Window& window, higanbana::GpuInfo info);
  int2 windowSize();
  void windowResized();
  void render(RendererOptions options, ActiveCamera viewMat, higanbana::vector<InstanceDraw>& instances, int cubeCount, int cubeCommandLists, std::optional<higanbana::CpuImage>& heightmap);
  std::optional<higanbana::SubmitTiming> timings();
  int loadMesh(MeshData& data);
  void unloadMesh(int index);
  int loadTexture(TextureData& data);
  void unloadTexture(int index);
  int loadMaterial(MaterialData& data);
  void unloadMaterial(int index);
};
}