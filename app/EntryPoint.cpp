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
    
    {
      GpuDevice gpu = devices.createGpuDevice(fs);
      renderdoc.startCapture();
      {
        constexpr int TestBufferSize = 1*128;

        auto testHeap = gpu.createMemoryHeap(HeapDescriptor()
          .setName("ebin")
          .sizeInBytes(32000000)
          .setHeapType(HeapType::Upload)); // 32megs, should be the common size...
        auto testHeap2 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinTarget").sizeInBytes(32000000).setHeapType(HeapType::Default)); // 32megs, should be the common size...
        auto testHeap3 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinReadback").sizeInBytes(32000000).setHeapType(HeapType::Readback)); // 32megs, should be the common size...
        auto buffer = gpu.createBuffer(testHeap,
          ResourceDescriptor()
            .Name("testBuffer")
			      .Format<float>()
            .Width(TestBufferSize)
            .Usage(ResourceUsage::UploadHeap)
            .Dimension(FormatDimension::Buffer));
		
        auto bufferTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
            .Name("testBufferTarget")
			      .Format<float>()
            .Width(TestBufferSize)
			.enableUnorderedAccess()
            .Usage(ResourceUsage::GpuOnly)
            .Dimension(FormatDimension::Buffer));
		auto bufferTargetUav = gpu.createBufferUAV(bufferTarget);
        auto computeTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
            .Name("testBufferTarget")
			      .Format<float>()
            .Width(TestBufferSize)
            .Usage(ResourceUsage::GpuOnly)
			.enableUnorderedAccess()
            .Dimension(FormatDimension::Buffer));
		auto computeTargetUav = gpu.createBufferUAV(computeTarget); 
        auto bufferReadb = gpu.createBuffer(testHeap3,
          ResourceDescriptor()
          .Name("testBufferTarget")
          .Format<float>()
          .Width(TestBufferSize)
          .Usage(ResourceUsage::ReadbackHeap)
          .Dimension(FormatDimension::Buffer));
		  
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

        for (int i = 0; i < 1; ++i)
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
              F_LOG("yay! mapped buffer! %f\n", map[TestBufferSize-1]);
              log.update();
            }
          }
        }
        renderdoc.endCapture();
      }
    }
  };
  main("w1");
  t.tick();
  t.printStatistics();
  log.update();
  return 0;
}
