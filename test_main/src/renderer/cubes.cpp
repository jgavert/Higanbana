#include "cubes.hpp"

#include <higanbana/core/math/math.hpp>
#include <higanbana/core/profiling/profiling.hpp>

SHADER_STRUCT(OpaqueConsts,
  float resx;
  float resy;
  float time;
  int stretchBoxes;
  float4x4 worldMat;
  float4x4 viewMat;
  float4 color;
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
    .setRTVFormat(0, FormatType::Float16RGBA)
    .setDSVFormat(FormatType::Depth32)
    .setRenderTargetCount(1)
    /*
    .setBlend(BlendDescriptor().setRenderTarget(0, RTBlendDescriptor()
        .setBlendEnable(true)
        .setBlendOpAlpha(BlendOp::Add)
        .setSrcBlend(Blend::SrcAlpha)
        .setSrcBlendAlpha(Blend::One)
        .setDestBlend(Blend::InvSrcAlpha)
        .setDestBlendAlpha(Blend::One)))*/
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(true)
      .setDepthFunc(ComparisonFunc::Greater));
  
  opaque = device.createGraphicsPipeline(opaqueDescriptor);

  opaqueRP = device.createRenderpass();
  opaqueRPWithLoad = device.createRenderpass();

  dir = float3(1, 0, 0);
  updir = float3(0, 1, 0);
  sideVec = float3(0, 0, 1);
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
        //consts.color = float4(pos.z, pos.y, pos.z, 1.f);
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

float mix(float x, float y, float a) {
  return x * (1.f - a) + y * x;
}
float3 mix(float3 x, float3 y, float3 a) {
  using namespace higanbana;
  float3 lol = sub(float3(1.f), a);
  float3 lol2 = mul(y, x);
  float3 lol3 = add(lol, lol2);
  float3 lol4 = mul(x, lol3);
  return lol4;
}

float smoothstep(float edge0, float edge1, float x) {
  float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

float4 heart(float2 fragCoord, float2 iResolution)
{
  using namespace higanbana;
  float2 p = math::div(math::sub(math::mul(2.0f, fragCoord), iResolution),std::min(iResolution.y,iResolution.x));
  
  p.y -= 0.3;

  // shape
  float a = std::atanf(p.y / p.x)/3.141593f;
  float r = length(p);
  float h = abs(a);
  float d = (13.0f*h - 22.0f*h*h + 10.0f*h*h*h)/(6.0f-5.0f*h);

  // color
  float s = 1.0f-0.5f*std::clamp(r/d,0.0f,1.0f);
  //s = 0.75 + 0.75*p.x;
  //s *= 1.0-0.25*r;
  //s = 0.5 + 0.6*s;
  //s *= 0.5+0.5*pow( 1.0-clamp(r/d, 0.0, 1.0 ), 0.1 );
  float3 hcol = float3(1.0f,0.0f,0.3f);//*s;
  
  float3 col = mix( float3(0.f,0.f,0.f), hcol, smoothstep( -0.01f, 0.01f, d-r) );

  return float4(col,1.0f);
}

css::Task<void> Cubes::oldOpaquePass2(higanbana::GpuGroup& dev, float time, higanbana::CommandGraphNode& node, float4x4 viewMat, higanbana::TextureRTV backbuffer, higanbana::TextureDSV depth, higanbana::DynamicBufferView ind,
    higanbana::ShaderArguments& args, int cubeCount, int xBegin, int xEnd){
  using namespace higanbana;
  std::string bname = "cubes ";
  bname += std::to_string(xEnd-xBegin);
  HIGAN_CPU_BRACKET(bname.c_str());
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


    //HIGAN_LOGi("%f %f %f %f\n", direction2.x, direction2.y, direction2.z, 0.01f * time);

    /*
    quaternion yaw = math::rotateAxis(updir, 0.1f*time*xDim);
    quaternion pitch = math::rotateAxis(sideVec, 0.1f*time*yDim);
    quaternion roll = math::rotateAxis(dir, 0.1f*time);
    quaternion direction2 = math::mul(math::mul(yaw, pitch), roll);
    auto rotationMatrix = math::rotationMatrixLH(direction2);
    auto worldMat = math::mul(math::scale(0.2f), rotationMatrix);
    */

    OpaqueConsts consts{};
    consts.time = time;
    consts.resx = backbuffer.desc().desc.width; 
    consts.resy = backbuffer.desc().desc.height;
    //consts.worldMat = math::mul(worldMat, math::translation(0,0,0));
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

      quaternion yaw = math::rotateAxis(updir, 0.1f*time*xDim);
      quaternion pitch = math::rotateAxis(sideVec, 0.1f*time*yDim);
      quaternion roll = math::rotateAxis(dir, 0.1f*time*zDim);
      quaternion direction2 = math::mul(math::mul(yaw, pitch), roll);
      auto rotationMatrix = math::rotationMatrixLH(direction2);
      auto worldMat = math::mul(math::scale(0.2f), rotationMatrix);
      
      float offset = 0.5;
      float3 pos = float3(xDim*offset, yDim*offset, zDim*offset);
      pos = math::sub(pos, 7.f);
      consts.worldMat = math::mul2(worldMat, math::translation(pos));
      //consts.color = heart(math::div(float2(zDim, yDim), float2(64, 64)), float2(64,64));
      consts.color = math::mul(float4(math::div(float2(zDim, yDim), float2(64, 64)), xDim/64.f, 1.f), sin(time)/2.f + 0.5f);
      
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