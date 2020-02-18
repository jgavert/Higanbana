#pragma once

#include "camera.hpp"
#include <higanbana/graphics/GraphicsCore.hpp>

namespace app
{
class Cubes 
{
  higanbana::ShaderArgumentsLayout triangleLayout;
  higanbana::GraphicsPipeline opaque;
  higanbana::Renderpass opaqueRP;
  higanbana::Renderpass opaqueRPWithLoad;
  float3 dir;
  float3 updir;
  float3 sideVec;
  quaternion direction;
public:
  Cubes(higanbana::GpuGroup& device);
  higanbana::ShaderArgumentsLayout& getLayout() { return triangleLayout;}
  void drawHeightMapInVeryStupidWay(
    higanbana::GpuGroup& dev,
    float time,
    higanbana::CommandGraphNode& node,
    float3 pos,
    float4x4 viewMat,
    higanbana::TextureRTV& backbuffer,
    higanbana::TextureDSV& depth,
    higanbana::CpuImage& image,
    int pixels,
    int xBegin, int xEnd);
  void drawHeightMapInVeryStupidWay2(
    higanbana::GpuGroup& dev,
    float time, 
    higanbana::CommandGraphNode& node,
    float3 pos, float4x4 viewMat,
    higanbana::TextureRTV& backbuffer,
    higanbana::TextureDSV& depth,
    higanbana::CpuImage& image,
    higanbana::DynamicBufferView ind,
    higanbana::ShaderArguments& verts,
    int pixels, int xBegin, int xEnd);
  void oldOpaquePass(higanbana::GpuGroup& dev, float time, higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, int cubeCount, int xBegin, int xEnd);
  void oldOpaquePass2(higanbana::GpuGroup& dev, float time, higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, higanbana::DynamicBufferView ind,
        higanbana::ShaderArguments& args, int cubeCount, int xBegin, int xEnd);
};
}