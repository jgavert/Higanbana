// EntryPoint.cpp
#include "Platform/EntryPoint.hpp"
#include "Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "Graphics/gfxApi.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/system/time.hpp"
#include "core/src/tests/schedulertests.hpp"
#include "graphics/apitests.hpp"
#include "graphics/apitests2.hpp"
#include "Graphics/SuperTest.hpp"
#include <cstdio>
#include <iostream>

using namespace faze;

int EntryPoint::main()
{
  // these tests will screw with directx frame capture
  {
    ApiTests tests;
    tests.run(m_params);
  }
  {
    ApiTests2 tests2;
    tests2.run(m_params);
  }
  {
    SchedulerTests::Run();
  }
  //return 1;
  

  auto main = [=](std::string name, std::string classn)
  {
    Logger log;
    WTime t;
    Window window(m_params, name, 800, 600);
    window.open();
    SystemDevices devices;
    GpuDevice gpu = devices.CreateGpuDevice(true);
    GpuCommandQueue queue = gpu.createQueue();
    GfxCommandList gfx = gpu.createUniversalCommandList();
    SwapChain sc = gpu.createSwapChain(queue, window);
    ViewPort port(800, 600);
    auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });
    MSG msg;


    // compute from examples
    ComputePipeline pipeline = gpu.createComputePipeline(ComputePipelineDescriptor().shader("compute_1.hlsl"));

    struct buf
    {
      int i;
      int k;
      int x;
      int y;
    };
    auto srcdata = gpu.createBufferSRV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Upload);
    auto dstdata = gpu.createBufferSRV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Gpu);
    auto completedata = gpu.createBufferUAV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Gpu);
    auto rbdata = gpu.createBufferSRV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Readback);

    {
      auto tmp = srcdata.buffer().Map<buf>();
      for (int i = 0;i < srcdata.buffer().size; ++i)
      {
        tmp[i].i = i;
        tmp[i].k = i;
      }
    }

    GpuFence fence = gpu.createFence();
    gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
    queue.submit(gfx);
    queue.insertFence(fence);
    
    // update loop
    t.firstTick();
    while (TRUE)
    {

      if (window.simpleReadMessages())
        break;
      fence.wait();
      gpu.resetCmdList(gfx);
      // compute begin
      gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
      auto bind = gfx.bind(pipeline);
      bind.SRV(0, dstdata);
      bind.UAV(0, completedata);
      size_t shaderGroup = 50;
      size_t inputSize = 1000 * 1000;
      gfx.Dispatch(bind, inputSize / shaderGroup, 1, 1);
      gfx.CopyResource(rbdata.buffer(), completedata.buffer());

      // Rendertarget
      gfx.setViewPort(port);
      vec[0] += 0.002f;
      if (vec[0] > 1.0f)
        vec[0] = 0.f;
      auto backBufferIndex = sc->GetCurrentBackBufferIndex();
      gfx.ClearRenderTargetView(sc[backBufferIndex], vec);

      // submit all
      queue.submit(gfx);

      // present
      sc->Present(1, 0);
      queue.insertFence(fence);
      t.tick();
    }
    t.printStatistics();
    log.update();
  };

  main("w1", "w1");
  

	return 1;
}
