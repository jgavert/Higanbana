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

#include <cstdio>
#include <iostream>


using namespace faze;

int EntryPoint::main()
{
  Logger log;
  GraphicsInstance devices;
  if (!devices.createInstance("faze"))
  {
    F_ILOG("System", "Failed to create Vulkan instance, exiting");
    log.update();
    return 1;
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
      GpuDevice gpu = devices.createGpuDevice();
      GraphicsQueue gfxQueue = gpu.createGraphicsQueue();
      DMAQueue dmaQueue = gpu.createDMAQueue();
      {
        GraphicsCmdBuffer gfx = gpu.createGraphicsCommandBuffer();
        DMACmdBuffer dma = gpu.createDMACommandBuffer();
        auto testHeap = gpu.createMemoryHeap(HeapDescriptor().setName("ebin").sizeInBytes(32000000)); // 32megs, should be the common size...
        auto buffer = gpu.createBuffer(testHeap,
          ResourceDescriptor()
            .Name("testBuffer")
            .Width(1000)
            .Usage(ResourceUsage::UploadHeap)
            .Dimension(FormatDimension::Buffer)
            .Format<float>());

        if (buffer.isValid())
        {
          F_LOG("yay! a buffer\n");
        }
      }
    }
    log.update();
  };

  main("w1");
  return 0;
}
