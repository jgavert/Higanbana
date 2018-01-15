#include "graphicsFixture.hpp"

#include "shaders/textureTest.if.hpp"
using namespace faze;
TEST_F(Graphics, BufferCreation)
{
  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Float32)
    .setWidth(32)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);
  auto bufferUAV = gpu.createBufferUAV(buffer);
}

TEST_F(Graphics, TextureCreation)
{
  auto testDesc = ResourceDescriptor()
    .setName("testTexture")
    .setFormat(FormatType::Unorm16RGBA)
    .setWidth(400)
    .setHeight(400)
    .setMiplevels(4)
    .setDimension(FormatDimension::Texture2D)
    .setUsage(ResourceUsage::RenderTargetRW);
  auto texture = gpu.createTexture(testDesc);
  auto texRtv = gpu.createTextureRTV(texture, ShaderViewDescriptor().setMostDetailedMip(0));
  auto texSrv = gpu.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
  auto texUav = gpu.createTextureUAV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
}

TEST_F(Graphics, RaymarchShader)
{
  ComputePipeline testCompute = gpu.createComputePipeline(ComputePipelineDescriptor()
    .setShader("textureTest")
    .setThreadGroups(uint3(8, 8, 1)));

  auto testDesc = ResourceDescriptor()
    .setName("testTexture")
    .setFormat(FormatType::Unorm16RGBA)
    .setWidth(400)
    .setHeight(400)
    .setMiplevels(4)
    .setDimension(FormatDimension::Texture2D)
    .setUsage(ResourceUsage::RenderTargetRW);
  auto texture = gpu.createTexture(testDesc);
  auto texRtv = gpu.createTextureRTV(texture, ShaderViewDescriptor().setMostDetailedMip(0));
  auto texSrv = gpu.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
  auto texUav = gpu.createTextureUAV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));

  uint2 iRes = uint2{ texUav.desc().desc.width, texUav.desc().desc.height };

  CommandGraph tasks = gpu.createGraph();
  {
    auto node = tasks.createPass("compute!");

    auto binding = node.bind<::shader::TextureTest>(testCompute);
    binding.constants = {};
    binding.constants.iResolution = iRes;
    binding.constants.iFrame = static_cast<int>(0);
    binding.constants.iTime = 0.f;
    binding.constants.iPos = float3(0);
    binding.constants.iDir = float3(1.f, 0, 0);
    binding.constants.iUpDir = float3(0, 1.f, 0);
    binding.constants.iSideDir = float3(0, 0, 1.f);
    binding.uav(::shader::TextureTest::output, texUav);

    unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(iRes.x), 8));
    unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(iRes.y), 8));
    node.dispatch(binding, uint3{ x, y, 1 });

    tasks.addPass(std::move(node));
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();
}