#include "graphicsFixture.hpp"

#include "blitter.hpp"
#include "shaders/textureTest.if.hpp"
#include "shaders/buffertest.if.hpp"
#include "shaders/triangle.if.hpp"
#include "shaders/posteffect.if.hpp"
#include "texture_pass.hpp"

using namespace faze;

TEST_F(Graphics, TexelStorageBuffer)
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

TEST_F(Graphics, TexelBuffer)
{
  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Float32)
    .setWidth(32);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);
}

TEST_F(Graphics, StorageBuffer)
{
  struct Lel
  {
    float a;
    int b;
    unsigned c;
  };

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setStructured<Lel>()
    .setWidth(32);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);
}

TEST_F(Graphics, StorageBufferRW)
{
  struct Lel
  {
    float a;
    int b;
    unsigned c;
  };

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setStructured<Lel>()
    .setWidth(32)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);
  auto bufferUAV = gpu.createBufferUAV(buffer);
}

TEST_F(Graphics, IndexBuffer)
{
  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Uint16)
    .setWidth(32)
    .setIndexBuffer();

  auto buffer = gpu.createBuffer(bufferdesc);
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

TEST_F(Graphics, UploadAndReadbackTest)
{
  ComputePipeline testBufferCompute = gpu.createComputePipeline<::shader::BufferTest>(ComputePipelineDescriptor()
    .setShader("buffertest")
    .setThreadGroups(uint3(32, 1, 1)));

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Uint32)
    .setWidth(32)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);

  bool WasReadback = false;

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

    node.readback(buffer, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i, asd[i]);
      }
      EXPECT_EQ(32, asd.size());
      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, readbackOffsetTest)
{
  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Uint32)
    .setWidth(64)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);

  bool WasReadback = false;

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("Buffertest");

    vector<unsigned> vertices;
    for (unsigned i = 0; i < 16; ++i)
    {
      vertices.push_back(static_cast<unsigned>(-1));
    }
    for (unsigned i = 0; i < 32; ++i)
    {
      vertices.push_back(i);
    }
    for (unsigned i = 0; i < 16; ++i)
    {
      vertices.push_back(static_cast<unsigned>(-1));
    }

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);

    node.readback(buffer, 16, 32, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i, asd[i]);
      }
      EXPECT_EQ(32, asd.size());

      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, ModifyDataInShaderAndReadback)
{
  ComputePipeline testBufferCompute = gpu.createComputePipeline<::shader::BufferTest>(ComputePipelineDescriptor()
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

  bool WasReadback = false;

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

    node.readback(buffer2, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i * 2, asd[i]);
      }
      EXPECT_EQ(32, asd.size());

      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, ModifyDataInShaderAndReadback_newbind)
{
	ComputePipeline testBufferCompute = gpu.createComputePipeline<::shader::BufferTest>(ComputePipelineDescriptor()
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

	bool WasReadback = false;

  // new binding

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

#if 1
		{
			auto binding = node.bind<::shader::BufferTest>(testBufferCompute);
			binding.srv(::shader::BufferTest::input, bufferSRV);
			binding.uav(::shader::BufferTest::output, bufferUAV2);
			node.dispatch(binding, uint3(1));
		}
#else
    {
      // new binding
      DescriptorSet set;
      {
        auto binding = DescriptorSetCreator<::shader::BufferTest>();
        binding.srv(::shader::BufferTest::input, bufferSRV);
        binding.uav(::shader::BufferTest::output, bufferUAV2);
        set = gpu.createDescriptorSet(binding);
      }
      node.dispatch(set, uint3(1));
    }
#endif

		node.readback(buffer2, [&WasReadback](MemView<uint8_t> view)
		{
			auto asd = reinterpret_memView<unsigned>(view);

			for (unsigned i = 0; i < 32; ++i)
			{
				EXPECT_EQ(i * 2, asd[i]);
			}
			EXPECT_EQ(32, asd.size());

			WasReadback = true;
		});
	}

	gpu.submit(tasks);
	gpu.waitGpuIdle();

	EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, RaymarchShader)
{
  ComputePipeline testCompute = gpu.createComputePipeline<::shader::TextureTest>(ComputePipelineDescriptor()
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

TEST_F(Graphics, ModifyDataInComputeQueue)
{
  ComputePipeline testBufferCompute = gpu.createComputePipeline<::shader::BufferTest>(ComputePipelineDescriptor()
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

  bool WasReadback = false;

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("Buffertest", CommandGraphNode::NodeType::Compute);

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

    node.readback(buffer2, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i * 2, asd[i]);
      }
      EXPECT_EQ(32, asd.size());

      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, CopyDataInDMA)
{
  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Uint32)
    .setWidth(64)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);

  bool WasReadback = false;

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("Buffertest", CommandGraphNode::NodeType::DMA);

    vector<unsigned> vertices;
    for (unsigned i = 0; i < 16; ++i)
    {
      vertices.push_back(static_cast<unsigned>(-1));
    }
    for (unsigned i = 0; i < 32; ++i)
    {
      vertices.push_back(i);
    }
    for (unsigned i = 0; i < 16; ++i)
    {
      vertices.push_back(static_cast<unsigned>(-1));
    }

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);

    node.readback(buffer, 16, 32, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i, asd[i]);
      }
      EXPECT_EQ(32, asd.size());

      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, ComputetoDMA)
{
  ComputePipeline testBufferCompute = gpu.createComputePipeline<::shader::BufferTest>(ComputePipelineDescriptor()
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

  bool WasReadback = false;

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("compute", CommandGraphNode::NodeType::Compute);

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
  }
  {
    auto& node = tasks.createPass2("readback", CommandGraphNode::NodeType::DMA);
    node.readback(buffer2, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i * 2, asd[i]);
      }
      EXPECT_EQ(32, asd.size());

      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, DMAtoCompute)
{
  ComputePipeline testBufferCompute = gpu.createComputePipeline<::shader::BufferTest>(ComputePipelineDescriptor()
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

  bool WasReadback = false;

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("uploadThroughDMA", CommandGraphNode::NodeType::DMA);

    vector<unsigned> vertices;
    for (unsigned i = 0; i < 32; ++i)
    {
      vertices.push_back(i);
    }

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);
  }
  {
    auto& node = tasks.createPass2("uploadThroughDMA", CommandGraphNode::NodeType::Compute);
    {
      auto binding = node.bind<::shader::BufferTest>(testBufferCompute);
      binding.srv(::shader::BufferTest::input, bufferSRV);
      binding.uav(::shader::BufferTest::output, bufferUAV2);
      node.dispatch(binding, uint3(1));
    }

    node.readback(buffer2, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i * 2, asd[i]);
      }
      EXPECT_EQ(32, asd.size());

      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, DMAtoComputetoDMA)
{
  ComputePipeline testBufferCompute = gpu.createComputePipeline<::shader::BufferTest>(ComputePipelineDescriptor()
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

  bool WasReadback = false;

  CommandGraph tasks = gpu.createGraph();

  auto dmaSema = gpu.createSemaphore();
  {
    auto& node = tasks.createPass2("uploadThroughDMA", {}, dmaSema, CommandGraphNode::NodeType::DMA);

    vector<unsigned> vertices;
    for (unsigned i = 0; i < 32; ++i)
    {
      vertices.push_back(i);
    }

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);
  }

  auto computeSema = gpu.createSemaphore();
  {
    auto& node = tasks.createPass2("doCompute", dmaSema, computeSema, CommandGraphNode::NodeType::Compute);
    {
      auto binding = node.bind<::shader::BufferTest>(testBufferCompute);
      binding.srv(::shader::BufferTest::input, bufferSRV);
      binding.uav(::shader::BufferTest::output, bufferUAV2);
      node.dispatch(binding, uint3(1));
    }
  }
  {
    auto& node = tasks.createPass2("readback", computeSema, {}, CommandGraphNode::NodeType::DMA);
    node.readback(buffer2, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<unsigned>(view);

      for (unsigned i = 0; i < 32; ++i)
      {
        EXPECT_EQ(i * 2, asd[i]);
      }
      EXPECT_EQ(32, asd.size());

      WasReadback = true;
    });
  }

  gpu.explicitSubmit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, Triangle)
{
  Renderpass triangleRenderpass = gpu.createRenderpass();

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setVertexShader("triangle")
    .setPixelShader("triangle")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false));

  GraphicsPipeline trianglePipe = gpu.createGraphicsPipeline<::shader::Triangle>(pipelineDescriptor);

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Float32RGBA)
    .setWidth(3)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);

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

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("upload");

    vector<float4> vertices;
    vertices.push_back(float4{ -1.f, -1.f, 1.f, 1.f });
    vertices.push_back(float4{ -1.0f, 3.f, 1.f, 1.f });
    vertices.push_back(float4{ 3.f, -1.f, 1.f, 1.f });

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);
  }
  {
    // we have pulsing red color background, draw a triangle on top of it !
    auto node = tasks.createPass("Triangle!");
    node.renderpass(triangleRenderpass);
    node.subpass(texRtv);
    texRtv.setOp(LoadOp::Clear);

    auto binding = node.bind<::shader::Triangle>(trianglePipe);
    //binding.constants.color = float4{ 0.f, 0.f, std::sin(float(frame)*0.01f + 1.0f)*.5f + .5f, 1.f };
    binding.constants.color = float4{ 0.f, 0.f, 0.f, 1.f };
    binding.srv(::shader::Triangle::vertices, bufferSRV);
    node.draw(binding, 3, 1);
    node.endRenderpass();
    tasks.addPass(std::move(node));
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();
}

TEST_F(Graphics, DMAtoGraphics)
{
  Renderpass triangleRenderpass = gpu.createRenderpass();

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setVertexShader("triangle")
    .setPixelShader("triangle")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false));

  GraphicsPipeline trianglePipe = gpu.createGraphicsPipeline<::shader::Triangle>(pipelineDescriptor);

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Float32RGBA)
    .setWidth(3)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);

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

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("uploadThroughDMA", CommandGraphNode::NodeType::DMA);

    vector<float4> vertices;
    vertices.push_back(float4{ -1.f, -1.f, 1.f, 1.f });
    vertices.push_back(float4{ -1.0f, 3.f, 1.f, 1.f });
    vertices.push_back(float4{ 3.f, -1.f, 1.f, 1.f });

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);
  }
  {
    // we have pulsing red color background, draw a triangle on top of it !
    auto node = tasks.createPass("Triangle!");
    node.renderpass(triangleRenderpass);
    node.subpass(texRtv);
    texRtv.setOp(LoadOp::Clear);

    auto binding = node.bind<::shader::Triangle>(trianglePipe);
    //binding.constants.color = float4{ 0.f, 0.f, std::sin(float(frame)*0.01f + 1.0f)*.5f + .5f, 1.f };
    binding.constants.color = float4{ 0.f, 0.f, 0.f, 1.f };
    binding.srv(::shader::Triangle::vertices, bufferSRV);
    node.draw(binding, 3, 1);
    node.endRenderpass();
    tasks.addPass(std::move(node));
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();
}

TEST_F(Graphics, GraphicstoDMA)
{
  Renderpass triangleRenderpass = gpu.createRenderpass();

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setVertexShader("triangle")
    .setPixelShader("triangle")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false));

  GraphicsPipeline trianglePipe = gpu.createGraphicsPipeline<::shader::Triangle>(pipelineDescriptor);

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Float32RGBA)
    .setWidth(3)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);

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

  bool WasReadback = false;

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("uploadThroughDMA");

    vector<float4> vertices;
    vertices.push_back(float4{ -1.f, -1.f, 1.f, 1.f });
    vertices.push_back(float4{ -1.0f, 3.f, 1.f, 1.f });
    vertices.push_back(float4{ 3.f, -1.f, 1.f, 1.f });

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    node.copy(buffer, verts);
    // we have pulsing red color background, draw a triangle on top of it !
  }
  {
    auto node = tasks.createPass("Triangle!");
    node.renderpass(triangleRenderpass);
    node.subpass(texRtv);
    texRtv.setOp(LoadOp::Clear);

    auto binding = node.bind<::shader::Triangle>(trianglePipe);
    //binding.constants.color = float4{ 0.f, 0.f, std::sin(float(frame)*0.01f + 1.0f)*.5f + .5f, 1.f };
    binding.constants.color = float4{ 0.f, 0.f, 0.f, 1.f };
    binding.srv(::shader::Triangle::vertices, bufferSRV);
    node.draw(binding, 3, 1);
    node.endRenderpass();
    tasks.addPass(std::move(node));
  }
  {
    auto& node = tasks.createPass2("readback", CommandGraphNode::NodeType::DMA);
    node.readback(buffer, [&WasReadback](MemView<uint8_t> view)
    {
      auto asd = reinterpret_memView<float4>(view);

      vector<float4> vertices;
      vertices.push_back(float4{ -1.f, -1.f, 1.f, 1.f });
      vertices.push_back(float4{ -1.0f, 3.f, 1.f, 1.f });
      vertices.push_back(float4{ 3.f, -1.f, 1.f, 1.f });

      for (size_t i = 0; i < vertices.size(); ++i)
      {
        EXPECT_FLOAT_EQ(vertices[i].x, asd[i].x);
        EXPECT_FLOAT_EQ(vertices[i].y, asd[i].y);
        EXPECT_FLOAT_EQ(vertices[i].z, asd[i].z);
        EXPECT_FLOAT_EQ(vertices[i].w, asd[i].w);
      }

      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();
  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, GraphicstoComputeToGraphics)
{
  Renderpass triangleRenderpass = gpu.createRenderpass();

  auto pipelineDescriptor = GraphicsPipelineDescriptor()
    .setVertexShader("triangle")
    .setPixelShader("triangle")
    .setPrimitiveTopology(PrimitiveTopology::Triangle)
    .setDepthStencil(DepthStencilDescriptor()
      .setDepthEnable(false));

  GraphicsPipeline trianglePipe = gpu.createGraphicsPipeline<::shader::Triangle>(pipelineDescriptor);

  renderer::TexturePass<::shader::PostEffect> postPassLDS(gpu, "posteffectLDS", uint3(64, 1, 1));
  renderer::Blitter blit(gpu);

  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Float32RGBA)
    .setWidth(3)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);

  auto testDesc = ResourceDescriptor()
    .setName("triangle")
    .setFormat(FormatType::Unorm16RGBA)
    .setWidth(400)
    .setHeight(400)
    .setDimension(FormatDimension::Texture2D)
    .setUsage(ResourceUsage::RenderTarget);
  auto texture = gpu.createTexture(testDesc);
  auto texRtv = gpu.createTextureRTV(texture);
  auto texSrv = gpu.createTextureSRV(texture);

  auto testDesc2 = ResourceDescriptor()
    .setName("blurredTriangle")
    .setFormat(FormatType::Unorm16RGBA)
    .setWidth(400)
    .setHeight(400)
    .setDimension(FormatDimension::Texture2D)
    .setUsage(ResourceUsage::GpuRW);
  auto texture2 = gpu.createTexture(testDesc2);
  auto tex2Uav = gpu.createTextureUAV(texture2);
  auto tex2Srv = gpu.createTextureSRV(texture2);

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("Triangle");

    vector<float4> vertices;
    vertices.push_back(float4{ -1.f, -1.f, 1.f, 1.f });
    vertices.push_back(float4{ -1.0f, 3.f, 1.f, 1.f });
    vertices.push_back(float4{ 3.f, -1.f, 1.f, 1.f });

    auto verts = gpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

    // we have pulsing red color background, draw a triangle on top of it !
    node.renderpass(triangleRenderpass);
    texRtv.setOp(LoadOp::Clear);
    node.subpass(texRtv);

    auto binding = node.bind<::shader::Triangle>(trianglePipe);
    binding.constants.color = float4{ 0.5f, 0.5f, 0.f, 1.f };
    binding.srv(::shader::Triangle::vertices, bufferSRV);
    node.draw(binding, 3, 1);

    node.resetState(texture);
    node.endRenderpass();
  }
  {
    auto& node = tasks.createPass2("compute1", CommandGraphNode::NodeType::Compute);

    postPassLDS.compute(node, tex2Uav, texSrv, uint2(testDesc.desc.width * 64, 1));
  }
  {
    auto& node = tasks.createPass2("blit in graphics");

    blit.blitImage(gpu, node, texRtv, tex2Srv, renderer::Blitter::FitMode::Fill);
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();
}

TEST_F(Graphics, TextureReadbackTest)
{
  CpuImage image(ResourceDescriptor()
    .setSize(int2(32, 32))
    .setFormat(FormatType::Uint32)
    .setName("test")
    .setUsage(ResourceUsage::GpuReadOnly));

  auto sr = image.subresource(0, 0);

  vector<uint> dataLol;
  dataLol.resize(32 * 32, 1337);

  memcpy(sr.data(), dataLol.data(), sr.size());

  bool WasReadback = false;

  auto testImage = gpu.createTexture(image);

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("Buffertest");

    node.readback(testImage, Subresource(), [&WasReadback](SubresourceData data)
    {
      F_LOG("zomg! %d %d %d\n", data.dim().x, data.dim().y, data.dim().z);
      EXPECT_EQ(32, data.dim().x);
      EXPECT_EQ(32, data.dim().y);
      EXPECT_EQ(1, data.dim().z);
      EXPECT_EQ(1337u, *reinterpret_cast<const unsigned*>(data.data()));
      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}

TEST_F(Graphics, TexturetoTexturetoReadbackTest)
{
  CpuImage image(ResourceDescriptor()
    .setSize(int2(32, 32))
    .setFormat(FormatType::Uint32)
    .setName("test")
    .setUsage(ResourceUsage::GpuReadOnly));
  auto sr = image.subresource(0, 0);
  vector<uint> dataLol;
  dataLol.resize(32 * 32, 1337);
  memcpy(sr.data(), dataLol.data(), sr.size());
  auto testImage = gpu.createTexture(image);

  bool WasReadback = false;
  auto anotherImage = gpu.createTexture(ResourceDescriptor()
    .setSize(int2(32, 32))
    .setFormat(FormatType::Uint32)
    .setName("test")
    .setUsage(ResourceUsage::GpuReadOnly));

  CommandGraph tasks = gpu.createGraph();
  {
    auto& node = tasks.createPass2("Buffertest");

    node.copy(anotherImage, testImage);

    node.readback(anotherImage, Subresource(), [&WasReadback](SubresourceData data)
    {
      F_LOG("zomg! %d %d %d\n", data.dim().x, data.dim().y, data.dim().z);
      EXPECT_EQ(32, data.dim().x);
      EXPECT_EQ(32, data.dim().y);
      EXPECT_EQ(1, data.dim().z);
      EXPECT_EQ(1337u, *reinterpret_cast<const unsigned*>(data.data()));
      WasReadback = true;
    });
  }

  gpu.submit(tasks);
  gpu.waitGpuIdle();

  EXPECT_TRUE(WasReadback);
}