// EntryPoint.cpp
#include "Platform/EntryPoint.hpp"
#include "Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "Graphics/gfxApi.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/system/time.hpp"
#include "graphics/apitests.hpp"
#include "graphics/apitests2.hpp"
#include "Graphics/SuperTest.hpp"
#include <cstdio>
#include <iostream>

using namespace faze;

int EntryPoint::main()
{
  
  ApiTests tests;
  tests.run();
  ApiTests2 tests2;
  tests2.run();
  
  //return 1;
  
  auto main = [=](std::string name, std::string classn)
  {
    Logger log;
    WTime t;
    Window window(m_params, classn);
    window.open(name, 800, 600);
    SystemDevices devices;
    GpuDevice gpu = devices.CreateGpuDevice(true);
    test tests(gpu);
    GpuCommandQueue queue = gpu.createQueue();
    GfxCommandList gfx = gpu.createUniversalCommandList();
    //GpuFence fence = gpu.createFence();
    //queue.submit(gfx);
    //queue.insertFence(fence);
    //fence.wait();
    //gpu.doExperiment();
    //auto res = gpu.CommittedResTest();
    //auto stuff = gpu.heapCreationTest();
    MSG msg;
    t.firstTick();
    while (TRUE)
    {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT)
          break;
      }
      tests.RunCompute(gpu, queue, gfx);
      t.tick();
      //tests.run();
    }
    t.printStatistics();
    log.update();
  };

  main("w1", "w1");
  

	return 1;
}
