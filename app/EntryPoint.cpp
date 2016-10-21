// EntryPoint.cpp
#ifdef WIN64
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/system/time.hpp"
#include "core/src/tests/schedulertests.hpp"
#include "core/src/tests/bitfield_tests.hpp"
#include "core/src/math/mat_templated.hpp"
#include "app/Graphics/gfxApi.hpp"
#include "core/src/filesystem/filesystem.hpp"

#include "core/src/system/memview.hpp"
#include "core/src/spirvcross/spirv_glsl.hpp"

#include "core/src/tools/renderdoc.hpp"

#include "vkShaders/sampleShader.if.hpp"


#include <shaderc/shaderc.hpp> 
#include <cstdio>
#include <iostream>
#include <sparsepp.h>

using namespace faze;

int EntryPoint::main()
{
  WTime t;
  t.firstTick();
  Logger log;

  GraphicsInstance devices;
  FileSystem fs;
  if (!devices.createInstance("faze"))
  {
    F_ILOG("System", "Failed to create Vulkan instance, exiting");
    log.update();
    return 1;
  }
  {
    //SchedulerTests::Run();
  }
  RenderDocApi renderdoc;
  auto main = [&](std::string name)
  {
    //LBS lbs;
    ivec2 ires = { 800, 600 };
    vec2 res = { static_cast<float>(ires.x()), static_cast<float>(ires.y()) };
    Window window(m_params, name, ires.x(), ires.y());
    window.open();
    int64_t frame = 1;
    {
      GpuDevice gpu = devices.createGpuDevice(fs);
      WindowSurface surface = devices.createSurface(window);
      auto swapchain = gpu.createSwapchain(surface, PresentMode::Mailbox);

      // TODO1: general, advance towards usable images
      // optional: cleanup texture format

      //renderdoc.startCapture();
      {
        constexpr int TestBufferSize = 1 * 128;

        auto testHeap = gpu.createMemoryHeap(HeapDescriptor()
          .setName("ebin")
          .sizeInBytes(32000000)
          .setHeapType(HeapType::Upload)); // 32megs, should be the common size...
        auto testHeap2 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinTarget").sizeInBytes(32000000).setHeapType(HeapType::Default)); // 32megs, should be the common size...
        auto testHeap3 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinReadback").sizeInBytes(32000000).setHeapType(HeapType::Readback)); // 32megs, should be the common size...
        auto buffer = gpu.createBuffer(testHeap,
          ResourceDescriptor()
          .setName("testBuffer")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .setUsage(ResourceUsage::UploadHeap)
          .setDimension(FormatDimension::Buffer));

        auto bufferTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .enableUnorderedAccess()
          .setUsage(ResourceUsage::GpuOnly)
          .setDimension(FormatDimension::Buffer));
        auto bufferTargetUav = gpu.createBufferUAV(bufferTarget);
        auto computeTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .setUsage(ResourceUsage::GpuOnly)
          .enableUnorderedAccess()
          .setDimension(FormatDimension::Buffer));
        auto computeTargetUav = gpu.createBufferUAV(computeTarget);
        auto bufferReadb = gpu.createBuffer(testHeap3,
          ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .setUsage(ResourceUsage::ReadbackHeap)
          .setDimension(FormatDimension::Buffer));


        auto testTexture = gpu.createTexture(testHeap2, ResourceDescriptor()
          .setName("TestTexture")
          .setWidth(800)
          .setHeight(600)
          .setFormat(FormatType::R8G8B8A8_Uint)
          .setDimension(FormatDimension::Texture2D)
          .setUsage(ResourceUsage::GpuOnly)
          .setLayout(TextureLayout::StandardSwizzle64kb));

        if (buffer.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = buffer.Map<float>(0, TestBufferSize);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer!\n");
              for (auto i = 0; i < TestBufferSize; ++i)
              {
                map[i] = 1.f;
              }
            }
          }
        }
        ComputePipeline test = gpu.createComputePipeline<SampleShader>(ComputePipelineDescriptor().shader("sampleShader"));
        {
          auto gfx = gpu.createGraphicsCommandBuffer();
          gfx.copy(buffer, bufferTarget);
          {
            auto shif = gfx.bind<SampleShader>(test);
            shif.read(SampleShader::dataIn, bufferTargetUav);
            shif.modify(SampleShader::dataOut, computeTargetUav);
            gfx.dispatchThreads(shif, TestBufferSize);
          }
          gpu.submit(gfx);
        }

        t.firstTick();

		float value = 0.f;

		while (!window.simpleReadMessages(frame++))
		{
      if (window.hasResized())
      {
        gpu.reCreateSwapchain(swapchain, surface);
        window.resizeHandled();
      }

      auto& inputs = window.inputs();
      if (inputs.isPressedThisFrame(VK_ESCAPE, 1))
      {
        // \o/ is work
        // mouse next, input capture in window?
        break;
      }


			log.update();
			{
				auto gfx = gpu.createGraphicsCommandBuffer();
				auto shif = gfx.bind<SampleShader>(test);
				for (int k = 0; k < 1; k++)
				{
					{
						shif.read(SampleShader::dataIn, computeTargetUav);
						shif.modify(SampleShader::dataOut, bufferTargetUav);
						gfx.dispatchThreads(shif, TestBufferSize);
					}
					{
						shif.read(SampleShader::dataIn, bufferTargetUav);
						shif.modify(SampleShader::dataOut, computeTargetUav);
						gfx.dispatchThreads(shif, TestBufferSize);
					}
				}
				gpu.submit(gfx);
			}
			{
				auto rtv = gpu.acquirePresentableImage(swapchain);
				{
					auto gfx = gpu.createGraphicsCommandBuffer();
					gfx.clearRTV(rtv, value);
					gpu.submitSwapchain(gfx, swapchain);
				}
				gpu.present(swapchain);
				t.tick();
				value += t.getFrameTimeDelta();
        if (value > 1.f)
          value = 0.f;
			}
		}

		// outside windowloop
        {
          auto gfx = gpu.createGraphicsCommandBuffer();
          gfx.copy(computeTarget, bufferReadb);
          gpu.submit(gfx);
        }
        gpu.waitIdle();

        if (bufferReadb.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = bufferReadb.Map<float>(0, TestBufferSize);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer! %f\n", map[TestBufferSize - 1]);
              log.update();
            }
          }
        }
        //renderdoc.endCapture();
      }
    }
  };
  main("w1");
  t.printStatistics();
  log.update();
  return 0;
}
