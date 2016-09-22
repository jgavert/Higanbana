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
#include "core/src/system/SequenceTracker.hpp"
#include "core/src/system/SequenceRingRangeAllocator.hpp"
#include "core/src/spirvcross/spirv_glsl.hpp"

#include "vkShaders/sampleShader.if.hpp"

#include <shaderc/shaderc.hpp> 
#include <cstdio>
#include <iostream>

using namespace faze;

int EntryPoint::main()
{
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
	  SchedulerTests::Run();
  }

  auto main = [&](std::string name)
  {
    //LBS lbs;
    WTime t;
    ivec2 ires = { 800, 600 };
    vec2 res = { static_cast<float>(ires.x()), static_cast<float>(ires.y()) };
    //Window window(m_params, name, ires.x(), ires.y());
    //window.open();
    
    {
      GpuDevice gpu = devices.createGpuDevice(fs);

      {
        ComputePipeline test = gpu.createComputePipeline<SampleShader>(ComputePipelineDescriptor().shader("sampleShader"));
        auto testHeap = gpu.createMemoryHeap(HeapDescriptor().setName("ebin").sizeInBytes(32000000).setHeapType(HeapType::Upload)); // 32megs, should be the common size...
        auto testHeap2 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinTarget").sizeInBytes(32000000).setHeapType(HeapType::Default)); // 32megs, should be the common size...
        //auto testHeap3 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinReadback").sizeInBytes(32000000).setHeapType(HeapType::Readback)); // 32megs, should be the common size...
        auto buffer = gpu.createBuffer(testHeap,
          ResourceDescriptor()
            .Name("testBuffer")
			      .Format<float>()
            .Width(1000)
            .Usage(ResourceUsage::UploadHeap)
            .Dimension(FormatDimension::Buffer));
		/*
        auto bufferTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
            .Name("testBufferTarget")
			      .Format<float>()
            .Width(1000)
            .Usage(ResourceUsage::GpuOnly)
            .Dimension(FormatDimension::Buffer));
			*/
        /*
        auto bufferReadb = gpu.createBuffer(testHeap3,
          ResourceDescriptor()
          .Name("testBufferTarget")
          .Format<float>()
          .Width(1000)
          .Usage(ResourceUsage::ReadbackHeap)
          .Dimension(FormatDimension::Buffer));
		  */
        if (buffer.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = buffer.Map<float>(0, 1000);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer!\n");
              map[0] = 1.f;
            }
          }
        }
        //gfx.copy(buffer, bufferTarget);
        //dma.copy(bufferTarget, bufferReadb);
        for (int i = 0; i < 20; ++i)
        {
          auto gfx = gpu.createGraphicsCommandBuffer();
          gpu.submit(gfx);
        }
        //gpu.waitIdle();
      }
    }
  };

  main("w1");
  return 0;
}
