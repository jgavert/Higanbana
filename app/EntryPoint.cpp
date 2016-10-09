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

#include "vkShaders/sampleShader.if.hpp"

#ifdef PLATFORM_WINDOWS
#include <renderdoc_app.h>
#endif
#include <shaderc/shaderc.hpp> 
#include <cstdio>
#include <iostream>

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
#ifdef PLATFORM_WINDOWS
  RENDERDOC_API_1_0_0 *rdoc_api = nullptr;
  if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
  {
    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");

    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void **)&rdoc_api);

    F_ASSERT(ret == 1,"");
    F_ASSERT(rdoc_api->StartFrameCapture != nullptr && rdoc_api->EndFrameCapture != nullptr, "");

    int major = 999, minor = 999, patch = 999;

    rdoc_api->GetAPIVersion(&major, &minor, &patch);

    F_ASSERT(major == 1 && minor >= 0 && patch >= 0, "");
  }
#endif
  auto main = [&](std::string name)
  {
    //LBS lbs;
    ivec2 ires = { 800, 600 };
    vec2 res = { static_cast<float>(ires.x()), static_cast<float>(ires.y()) };
    //Window window(m_params, name, ires.x(), ires.y());
    //window.open();
    
    {
      GpuDevice gpu = devices.createGpuDevice(fs);
#ifdef PLATFORM_WINDOWS
      if (rdoc_api)
      {
        rdoc_api->StartFrameCapture(nullptr, nullptr);
      }
#endif
      {
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
            .Width(100)
            .Usage(ResourceUsage::UploadHeap)
            .Dimension(FormatDimension::Buffer));
		
        auto bufferTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
            .Name("testBufferTarget")
			      .Format<float>()
            .Width(100)
			.enableUnorderedAccess()
            .Usage(ResourceUsage::GpuOnly)
            .Dimension(FormatDimension::Buffer));
		auto bufferTargetUav = gpu.createBufferUAV(bufferTarget);
        auto computeTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
            .Name("testBufferTarget")
			      .Format<float>()
            .Width(100)
            .Usage(ResourceUsage::GpuOnly)
			.enableUnorderedAccess()
            .Dimension(FormatDimension::Buffer));
		auto computeTargetUav = gpu.createBufferUAV(computeTarget); 
        auto bufferReadb = gpu.createBuffer(testHeap3,
          ResourceDescriptor()
          .Name("testBufferTarget")
          .Format<float>()
          .Width(100)
          .Usage(ResourceUsage::ReadbackHeap)
          .Dimension(FormatDimension::Buffer));
		  
        if (buffer.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = buffer.Map<float>(0, 100);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer!\n");
              for (auto i = 0; i < 100; ++i)
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
            gfx.dispatch(shif, 1, 1, 1);
          }
          gpu.submit(gfx);
        }
        for (int i = 0; i < 10; ++i)
        {
          auto gfx = gpu.createGraphicsCommandBuffer();
          auto shif = gfx.bind<SampleShader>(test);
          for (int k = 0; k < 4; k++)
          {
            {
              shif.read(SampleShader::dataIn, computeTargetUav);
              shif.modify(SampleShader::dataOut, bufferTargetUav);
              gfx.dispatch(shif, 1, 1, 1);
            }
            {
              shif.read(SampleShader::dataIn, bufferTargetUav);
              shif.modify(SampleShader::dataOut, computeTargetUav);
              gfx.dispatch(shif, 1, 1, 1);
            }
          }
          gfx.copy(computeTarget, bufferReadb);
          gpu.submit(gfx);
        }
        gpu.waitIdle();

        if (bufferReadb.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = bufferReadb.Map<float>(0, 100);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer! %f\n", map[0]);
              log.update();
            }
          }
        }
#ifdef PLATFORM_WINDOWS
        if (rdoc_api)
        {
          rdoc_api->EndFrameCapture(nullptr, nullptr);
        }
#endif
      }
    }
  };
  main("w1");
  t.tick();
  t.printStatistics();
  log.update();
  return 0;
}
