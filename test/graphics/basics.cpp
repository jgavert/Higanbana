#include "graphicsFixture.hpp"

#include "shaders/textureTest.if.hpp"
#include "shaders/buffertest.if.hpp"

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

TEST_F(Graphics, ModifyBufferDispatch)
{
  ComputePipeline testBufferCompute = gpu.createComputePipeline(ComputePipelineDescriptor()
    .setShader("buffertest")
    .setThreadGroups(uint3(32, 1, 1)));

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Uint32)
    .setWidth(32)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);
  auto bufferUAV = gpu.createBufferUAV(buffer);

  auto bufferdesc2 = ResourceDescriptor(bufferdesc)
    .setName("testBufferTarget2");

  auto buffer2 = gpu.createBuffer(bufferdesc2);
  auto bufferUAV2 = gpu.createBufferUAV(buffer2);

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("Buffertest");

    vector<unsigned> vertices;
    for (unsigned i = 0; i < 32; ++i)
    {
      vertices.push_back(i);
    }

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);

    {
      auto binding = node.bind<::shader::BufferTest>(testBufferCompute);
      binding.srv(::shader::BufferTest::input, bufferSRV);
      binding.uav(::shader::BufferTest::output, bufferUAV2);
      node.dispatch(binding, uint3(1));
    }

    node.readback(buffer2, [](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i * 2, asd[i]);
      }
      EXPECT_EQ(32, asd.size());
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();
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