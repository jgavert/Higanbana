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
#include "world/visual_data_structures.hpp"


namespace app
{
struct MeshViews
{
  higanbana::BufferIBV indices;
  higanbana::ShaderArguments args;
  higanbana::ShaderArguments meshArgs;
  higanbana::BufferSRV vertices;
  higanbana::BufferSRV uvs;
  higanbana::BufferSRV normals;
  // mesh shader required
  higanbana::BufferSRV uniqueIndices;
  higanbana::BufferSRV packedIndices;
  higanbana::BufferSRV meshlets;
};

class MeshSystem
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<MeshViews> views;

public:
  int allocate(higanbana::GpuGroup& gpu, higanbana::ShaderArgumentsLayout& normalLayout,higanbana::ShaderArgumentsLayout& meshLayout, MeshData& data);
  MeshViews& operator[](int index) { return views[index]; }
  void free(int index);
};

struct MaterialViews
{
  higanbana::TextureSRV albedo;
};

class MaterialDB
{
  higanbana::FreelistAllocator freelist;
  higanbana::vector<MaterialViews> views;

public:
  int allocate(higanbana::GpuGroup& gpu, higanbana::ShaderArgumentsLayout& materialLayout, MaterialData& data);
  MaterialViews& operator[](int index) { return views[index]; }
  void free(int index);
};

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
  // resources
  higanbana::ShaderArgumentsLayout triangleLayout;
  higanbana::GraphicsPipeline triangle;

  higanbana::Renderpass triangleRP;

  higanbana::GraphicsPipeline opaque;
  higanbana::Renderpass opaqueRP;
  higanbana::Renderpass opaqueRPWithLoad;

  higanbana::PingPongTexture proxyTex;
  higanbana::ShaderArgumentsLayout compLayout;
  higanbana::ComputePipeline genTexCompute;

  higanbana::ShaderArgumentsLayout blitLayout;
  higanbana::GraphicsPipeline composite;
  higanbana::Renderpass compositeRP;
  // info
  higanbana::WTime time;

  // shared textures
  higanbana::PingPongTexture targetRT;
  higanbana::Buffer sBuf;
  higanbana::BufferSRV sBufSRV;
  
  higanbana::Texture sTex;

  renderer::IMGui imgui;

  higanbana::CpuImage image;
  higanbana::Texture fontatlas;

  // gbuffer??
  higanbana::Texture depth;
  higanbana::TextureDSV depthDSV;

  // meshes
  MeshSystem meshes;
  WorldRenderer worldRend;
  MeshTest worldMeshRend;
  higanbana::Buffer cameras;
  higanbana::BufferSRV cameraSRV;
  higanbana::BufferUAV cameraUAV;
  higanbana::ShaderArguments staticDataArgs;

  // camera
  float3 position;
  float3 dir;
  float3 updir;
  float3 sideVec;
  quaternion direction;

  std::optional<higanbana::SubmitTiming> m_previousInfo;

  void drawHeightMapInVeryStupidWay(higanbana::CommandGraphNode& node, float3 pos, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, higanbana::CpuImage& image, int pixels, int xBegin, int xEnd);
  void drawHeightMapInVeryStupidWay2(
      higanbana::CommandGraphNode& node,
        float3 pos, float4x4 viewMat,
        higanbana::TextureRTV& backbuffer,
        higanbana::TextureDSV& depth,
        higanbana::CpuImage& image,
        higanbana::DynamicBufferView ind,
        higanbana::ShaderArguments& verts,
        int pixels, int xBegin, int xEnd);
  void oldOpaquePass(higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, int cubeCount, int xBegin, int xEnd);
  void oldOpaquePass2(higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, higanbana::DynamicBufferView ind,higanbana::ShaderArguments& verts, int cubeCount, int xBegin, int xEnd);
  void renderMeshes(higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances);
  void renderMeshesWithMeshShaders(higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::vector<InstanceDraw>& instances);

public:
  Renderer(higanbana::GraphicsSubsystem& graphics, higanbana::GpuGroup& dev);
  void initWindow(higanbana::Window& window, higanbana::GpuInfo info);
  int2 windowSize();
  void windowResized();
  void render(RendererOptions options, ActiveCamera viewMat, higanbana::vector<InstanceDraw>& instances, int cubeCount, int cubeCommandLists, std::optional<higanbana::CpuImage>& heightmap);
  std::optional<higanbana::SubmitTiming> timings();

  int loadMesh(MeshData& data);
  void unloadMesh(int index);
};
}