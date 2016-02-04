// EntryPoint.cpp
#include "Platform/EntryPoint.hpp"
#include "Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/system/time.hpp"
#include "core/src/tests/schedulertests.hpp"
#include "Graphics/gfxApi.hpp"
#include "graphics/tests/apitests.hpp"
#include "graphics/tests/apitests2.hpp"
#include "graphics/tests/advtests.hpp"
#include "graphics/tests/stresstests.hpp"

// app
#include "Rendering/Utils/graph.hpp"

#include <cstdio>
#include <iostream>

using namespace faze;

int EntryPoint::main()
{
  // these tests will screw with directx frame capture
  /*
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
  */
  //return 1;
 
  

  auto main = [=](std::string name)
  {
    LBS lbs;
    Logger log;
    WTime t;
    vec2 res = { 800.f, 600.f };
    Window window(m_params, name, res.x(), res.y());
    window.open();
    SystemDevices devices;
    GpuDevice gpu = devices.CreateGpuDevice(true);
    GpuCommandQueue queue = gpu.createQueue();
    SwapChain sc = gpu.createSwapChain(queue, window, 2, R8G8B8A8_UNORM_SRGB);
    ViewPort port(res.x(), res.y());

    GfxCommandList gfx = gpu.createUniversalCommandList();
    {
      // recommended only in release mode with debugging layer off...
      // didn't bother to find algorithm that would skip towards the right value.
      //StressTests::run(gpu, queue, window, sc, port, gfx, log);
      //AdvTests::run(gpu, queue, window, sc, port, gfx);
    }

    using namespace rendering::utils;

    Graph graph(gpu, -1.f, 1.f, ivec2({ 1600,600 }));

    auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

    // graphics 

    auto pipeline2 = gpu.createGraphicsPipeline(GraphicsPipelineDescriptor()
      .PixelShader("pixel2.hlsl")
      .VertexShader("vertex_triangle.hlsl")
      .setRenderTargetCount(1)
      .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
      .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

    struct ConstantsCustom
    {
      mat4 WorldMatrix;
      mat4 ViewMatrix;
      mat4 ProjectionMatrix;
      float time;
      vec2 resolution;
      float filler;
    };

	  auto srcConstants = gpu.createConstantBuffer(Dimension(1), Format<ConstantsCustom>(), ResUsage::Upload);
    auto dstConstants = gpu.createConstantBuffer(Dimension(1), Format<ConstantsCustom>(), ResUsage::Gpu);

    {
      auto m = srcConstants.buffer().Map<ConstantsCustom>();
      m[0].resolution = res;
    }

    gfx.CopyResource(dstConstants.buffer(), srcConstants.buffer());


    GpuFence fence = gpu.createFence();
    gfx.close();
    queue.submit(gfx);
    queue.insertFence(fence);
    
    // update loop
    t.firstTick();
    float time = 0.f;
    while (true)
    {
      if (window.simpleReadMessages())
        break;

      lbs.addTask("StartFrame", [&](size_t, size_t)
      {
        fence.wait();
        gfx.resetList();
      });

      lbs.addTask("FillCommandlists", { "StartFrame" }, {}, [&](size_t, size_t)
      {
        GpuProfilingBracket(queue, "Frame");
        {
          graph.updateGraphCompute(gfx, std::sinf(time));
        }

        {
          GpuProfilingBracket(gfx, "Updating Constants");
          {
            auto m = srcConstants.buffer().Map<ConstantsCustom>();
            m[0].resolution = res;
            m[0].time = time;
          }
          gfx.CopyResource(dstConstants.buffer(), srcConstants.buffer());
        }
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        {
          GpuProfilingBracket(gfx, "Clearing&Setting RTV");
          gfx.setViewPort(port);
          gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
          gfx.setRenderTarget(sc[backBufferIndex]);
        }

        {
          GpuProfilingBracket(gfx, "DrawTriangle");
          auto bind = gfx.bind(pipeline2);
          bind.CBV(0, dstConstants);
          gfx.drawInstanced(bind, 3, 1, 0, 0);
        }

        { // post process
          graph.drawGraph(gfx);
        }
        // submit all
        gfx.preparePresent(sc[backBufferIndex]);
      });

      lbs.addTask("Submit&Present", { "FillCommandlists" }, {}, [&](size_t, size_t)
      {
        gfx.close();
        queue.submit(gfx);

        // present
        {
          GpuProfilingBracket(queue, "Presenting frame");
          sc->Present(1, 0);
        }
        queue.insertFence(fence);
      });

      lbs.sleepTillKeywords({ "Submit&Present" });
      t.tick();
      time += t.getFrameTimeDelta();
	    log.update();
    }
    t.printStatistics();
    log.update();
  };

  main("w1");
  return 1;
}
