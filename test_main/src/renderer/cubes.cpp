#include "cubes.hpp"

#include <higanbana/core/math/math.hpp>

SHADER_STRUCT(OpaqueConsts,
  float resx;
  float resy;
  float time;
  int stretchBoxes;
  float4x4 worldMat;
  float4x4 viewMat;
);

namespace app::renderer
{
Cubes::Cubes(higanbana::GpuGroup& device){
  using namespace higanbana;
  higanbana::ShaderArgumentsLayoutDescriptor triangleLayoutDesc = ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::ByteAddressBuffer, "vertexInput");
  triangleLayout = device.createShaderArgumentsLayout(triangleLayoutDesc);
  higanbana::PipelineInterfaceDescriptor opaquePassInterface = PipelineInterfaceDescriptor()
    .constants<OpaqueConsts>()
    .shaderArguments(0, triangleLayout);

  auto opaqueDescriptor = GraphicsPipelineDescriptor()
    .setVertexShader("opaquePass")
    .setPixelShader("opaquePass")
    .setInterface(opaquePassInterface)
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setRTVFormat(0, FormatType::Unorm8BGRA)
    .setDSVFormat(FormatType::Depth32)
    .setRenderTargetCount(1)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(true)
      .setDepthFunc(ComparisonFunc::Greater));
  
  opaque = device.createGraphicsPipeline(opaqueDescriptor);

  opaqueRP = device.createRenderpass();
  opaqueRPWithLoad = device.createRenderpass();

  direction = { 1.f, 0.f, 0.f, 0.f };
}
void Cubes::drawHeightMapInVeryStupidWay(higanbana::GpuGroup& dev, float time, higanbana::CommandGraphNode& node, float3 pos, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, higanbana::CpuImage& image, int pixels, int xBegin, int xEnd){
  using namespace higanbana;
  float redcolor = std::sin(time)*.5f + .5f;
  //backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 1.f });
  auto rp = opaqueRP;
  if (depth.loadOp() == LoadOp::Load)
  {
    rp = opaqueRPWithLoad;
  }
  node.renderpass(rp, backbuffer, depth);
  {
    auto binding = node.bind(opaque);

    vector<float> vertexData = {
      1.0f, -1.f, -1.f,
      1.0f, -1.f, 1.f, 
      1.0f, 1.f, -1.f,
      1.0f, 1.f, 1.f,
      -1.0f, -1.f, -1.f,
      -1.0f, -1.f, 1.f, 
      -1.0f, 1.f, -1.f,
      -1.0f, 1.f, 1.f,
    };

    auto vert = dev.dynamicBuffer<float>(vertexData, FormatType::Raw32);
    vector<uint16_t> indexData = {
      1, 0, 2,
      1, 2, 3,
      5, 4, 0,
      5, 0, 1,
      7, 2, 6,
      7, 3, 2,
      5, 6, 4,
      5, 7, 6,
      6, 0, 4,
      6, 2, 0,
      7, 5, 1,
      7, 1, 3,
    };
    auto ind = dev.dynamicBuffer<uint16_t>(indexData, FormatType::Uint16);

    quaternion yaw = math::rotateAxis(updir, 0.f);
    quaternion pitch = math::rotateAxis(sideVec, 0.f);
    quaternion roll = math::rotateAxis(dir, 0.f);
    auto dir = math::mul(math::mul(math::mul(yaw, pitch), roll), direction);

    auto rotationMatrix = math::rotationMatrixLH(dir);
    auto worldMat = math::mul(math::scale(0.5f), rotationMatrix);

    OpaqueConsts consts{};
    consts.time = time;
    consts.resx = backbuffer.desc().desc.width; 
    consts.resy = backbuffer.desc().desc.height;
    consts.worldMat = math::mul(worldMat, math::translation(0,0,0));
    consts.viewMat = viewMat;
    consts.stretchBoxes = 1;
    binding.constants(consts);

    auto args = dev.createShaderArguments(ShaderArgumentsDescriptor("heightmap", triangleLayout)
      .bind("vertexInput", vert));

    binding.arguments(0, args);

    int gridSize = image.desc().desc.width;
    auto subr = image.subresource(0,0);
    float* data = reinterpret_cast<float*>(subr.data());

    float offset = 0.2f;
    xBegin *= pixels;
    xEnd *= pixels;

    auto yPixels = (xEnd - xBegin);
    for (int x = xBegin; x < xEnd; ++x)
    {
      for (int y = 0; y < yPixels; ++y)
      {
        auto pX = std::min(gridSize, std::max(0, static_cast<int>(x - (pos.x+(yPixels/2)))));
        auto pY = std::min(gridSize, std::max(0, static_cast<int>(y - (pos.y+(yPixels/2)))));
        int dataOffset = pY*gridSize + pX;
        float3 pos = float3(pX, pY, data[dataOffset]);
        consts.worldMat = math::mul2(worldMat, math::translation(pos));
        binding.constants(consts);
        node.drawIndexed(binding, ind, 36);
      }
    }
  }
  node.endRenderpass();
}
void Cubes::drawHeightMapInVeryStupidWay2(
    higanbana::GpuGroup& dev,
    float time, 
    higanbana::CommandGraphNode& node,
    float3 pos, float4x4 viewMat,
    higanbana::TextureRTV& backbuffer,
    higanbana::TextureDSV& depth,
    higanbana::CpuImage& image,
    higanbana::DynamicBufferView ind,
    higanbana::ShaderArguments& verts,
    int pixels, int xBegin, int xEnd){
  using namespace higanbana;
  float redcolor = std::sin(time)*.5f + .5f;

  //backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 1.f });
  auto rp = opaqueRP;
  if (depth.loadOp() == LoadOp::Load)
  {
    rp = opaqueRPWithLoad;
  }
  node.renderpass(rp, backbuffer, depth);
  {
    auto binding = node.bind(opaque);

    quaternion yaw = math::rotateAxis(updir, 0.f);
    quaternion pitch = math::rotateAxis(sideVec, 0.f);
    quaternion roll = math::rotateAxis(dir, 0.f);
    auto dir = math::mul(math::mul(math::mul(yaw, pitch), roll), direction);

    auto rotationMatrix = math::rotationMatrixLH(dir);
    auto worldMat = math::mul(math::scale(0.5f), rotationMatrix);

    OpaqueConsts consts{};
    consts.time = time;
    consts.resx = backbuffer.desc().desc.width; 
    consts.resy = backbuffer.desc().desc.height;
    consts.worldMat = math::mul(worldMat, math::translation(0,0,0));
    consts.viewMat = viewMat;
    consts.stretchBoxes = 1;
    binding.constants(consts);

    auto args = verts;

    binding.arguments(0, args);

    int gridSize = image.desc().desc.width;
    auto subr = image.subresource(0,0);
    float* data = reinterpret_cast<float*>(subr.data());

    float offset = 0.2f;
    xBegin *= pixels;
    xEnd *= pixels;

    auto yPixels = (xEnd - xBegin);
    for (int x = xBegin; x < xEnd; ++x)
    {
      for (int y = 0; y < yPixels; ++y)
      {
        auto pX = std::min(gridSize, std::max(0, static_cast<int>(x - (pos.x+(yPixels/2)))));
        auto pY = std::min(gridSize, std::max(0, static_cast<int>(y - (pos.y+(yPixels/2)))));
        int dataOffset = pY*gridSize + pX;
        float3 pos = float3(pX, pY, data[dataOffset]);
        consts.worldMat = math::mul2(worldMat, math::translation(pos));
        binding.constants(consts);
        node.drawIndexed(binding, ind, 36);
      }
    }
  }
  node.endRenderpass();
}
void Cubes::oldOpaquePass(higanbana::GpuGroup& dev, float time, higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV& backbuffer, higanbana::TextureDSV& depth, int cubeCount, int xBegin, int xEnd){
  using namespace higanbana;
  float redcolor = std::sin(time)*.5f + .5f;

  //backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 1.f });
  auto rp = opaqueRP;
  if (depth.loadOp() == LoadOp::Load)
  {
    rp = opaqueRPWithLoad;
  }
  node.renderpass(rp, backbuffer, depth);
  {
    auto binding = node.bind(opaque);

    vector<float> vertexData = {
      1.0f, -1.f, -1.f,
      1.0f, -1.f, 1.f, 
      1.0f, 1.f, -1.f,
      1.0f, 1.f, 1.f,
      -1.0f, -1.f, -1.f,
      -1.0f, -1.f, 1.f, 
      -1.0f, 1.f, -1.f,
      -1.0f, 1.f, 1.f,
    };

    auto vert = dev.dynamicBuffer<float>(vertexData, FormatType::Raw32);
    vector<uint16_t> indexData = {
      1, 0, 2,
      1, 2, 3,
      5, 4, 0,
      5, 0, 1,
      7, 2, 6,
      7, 3, 2,
      5, 6, 4,
      5, 7, 6,
      6, 0, 4,
      6, 2, 0,
      7, 5, 1,
      7, 1, 3,
    };
    auto ind = dev.dynamicBuffer<uint16_t>(indexData, FormatType::Uint16);

    quaternion yaw = math::rotateAxis(updir, 0.001f);
    quaternion pitch = math::rotateAxis(sideVec, 0.f);
    quaternion roll = math::rotateAxis(dir, 0.f);
    direction = math::mul(math::mul(math::mul(yaw, pitch), roll), direction);

    auto rotationMatrix = math::rotationMatrixLH(direction);
    auto worldMat = math::mul(math::scale(0.2f), rotationMatrix);

    OpaqueConsts consts{};
    consts.time = time;
    consts.resx = backbuffer.desc().desc.width; 
    consts.resy = backbuffer.desc().desc.height;
    consts.worldMat = math::mul(worldMat, math::translation(0,0,0));
    consts.viewMat = viewMat;
    binding.constants(consts);

    auto args = dev.createShaderArguments(ShaderArgumentsDescriptor("old opaque", triangleLayout)
      .bind("vertexInput", vert));

    binding.arguments(0, args);

    int gridSize = cubeCount;
    for (int x = xBegin; x < xEnd; ++x)
    {
      for (int y = 0; y < gridSize; ++y)
      {
        for (int z = 0; z < gridSize; ++z)
        {
          float offset = gridSize*1.5f/2.f;
          float3 pos = float3(x*1.5f-offset, y*1.5f-offset, z*1.5f-offset);
          consts.worldMat = math::mul2(worldMat, math::translation(pos));
          binding.constants(consts);
          node.drawIndexed(binding, ind, 36);
        }
      }
    }
  }
  node.endRenderpass();
}
higanbana::coro::StolenTask<void> Cubes::oldOpaquePass2(higanbana::GpuGroup& dev, float time, higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV backbuffer, higanbana::TextureDSV depth, higanbana::DynamicBufferView ind,
    higanbana::ShaderArguments& args, int cubeCount, int xBegin, int xEnd){
  using namespace higanbana;
  float redcolor = std::sin(time)*.5f + .5f;

  //backbuffer.clearOp(float4{ 0.f, redcolor, 0.f, 1.f });
  auto rp = opaqueRP;
  if (depth.loadOp() == LoadOp::Load)
  {
    rp = opaqueRPWithLoad;
  }
  node.renderpass(rp, backbuffer, depth);
  {
    auto binding = node.bind(opaque);

    quaternion yaw = math::rotateAxis(updir, 0.001f);
    quaternion pitch = math::rotateAxis(sideVec, 0.f);
    quaternion roll = math::rotateAxis(dir, 0.f);
    direction = math::mul(math::mul(math::mul(yaw, pitch), roll), direction);

    auto rotationMatrix = math::rotationMatrixLH(direction);
    auto worldMat = math::mul(math::scale(0.2f), rotationMatrix);

    OpaqueConsts consts{};
    consts.time = time;
    consts.resx = backbuffer.desc().desc.width; 
    consts.resy = backbuffer.desc().desc.height;
    consts.worldMat = math::mul(worldMat, math::translation(0,0,0));
    consts.viewMat = viewMat;
    binding.constants(consts);

    binding.arguments(0, args);

    int gridSize = 64;
    for (int index = xBegin; index < xEnd; ++index)
    {
      int xDim = index / (gridSize*gridSize);
      int indexWithin = index % (gridSize*gridSize);
      int yDim = indexWithin / gridSize;
      int zDim = indexWithin % gridSize;
      
      float offset = 0.5;
      float3 pos = float3(xDim*offset, yDim*offset, zDim*offset);
      pos = math::sub(pos, 7.f);
      consts.worldMat = math::mul2(worldMat, math::translation(pos));
      binding.constants(consts);
      node.drawIndexed(binding, ind, 36);
    }
    /*
    for (int x = xBegin; x < xEnd; ++x)
    {
      for (int y = 0; y < gridSize; ++y)
      {
        for (int z = 0; z < gridSize; ++z)
        {
          float offset = gridSize*1.5f/2.f;
          float3 pos = float3(x*1.5f-offset, y*1.5f-offset, z*1.5f-offset);
          consts.worldMat = math::mul2(worldMat, math::translation(pos));
          binding.constants(consts);
          node.drawIndexed(binding, ind, 36);
        }
      }
    }
    */
  }
  node.endRenderpass();
  co_return;
}
}