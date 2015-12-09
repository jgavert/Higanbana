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

    Graph graph(gpu, -1.f, 1.f, ivec2({ 800,200 }));

    //graph.changeScreenPos(vec2}))
    graph.updateGraphCompute(gfx, 0.5f);

    auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });
    // compute from examples
    ComputePipeline pipeline = gpu.createComputePipeline(ComputePipelineDescriptor()
      .shader("compute_1.hlsl"));

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
    gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
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

      fence.wait();
      gfx.resetList();

      {
        GpuProfilingBracket(queue, "Frame");
        {
          graph.updateGraphCompute(gfx, std::sinf(time));
        }
        { // set heaps 
          gfx.setHeaps(gpu.getDescHeaps());
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
        {
          GpuProfilingBracket(gfx, "Clearing&Setting RTV");
          gfx.setViewPort(port);
          auto backBufferIndex = sc->GetCurrentBackBufferIndex();
          gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
          gfx.setRenderTarget(sc[backBufferIndex]);
        }

        {
          GpuProfilingBracket(gfx, "Copy the compute job");
          gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
        }

        {
          GpuProfilingBracket(gfx, "Dispatch");
          auto bind = gfx.bind(pipeline);
          bind.SRV(0, dstdata);
          bind.UAV(0, completedata);
          unsigned int shaderGroup = 50;
          unsigned int inputSize = 1000 * 1000;
          gfx.Dispatch(bind, inputSize / shaderGroup, 1, 1);
        }

        {
          GpuProfilingBracket(gfx, "Copy dispatch results");
          gfx.CopyResource(rbdata.buffer(), completedata.buffer());
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
        gfx.close();
        queue.submit(gfx);

        // present
        {
          GpuProfilingBracket(queue, "Presenting frame");
          sc->Present(1, 0);
        }
      }
      queue.insertFence(fence);
      t.tick();
      time += t.getFrameTimeDelta();
    }
    t.printStatistics();
    log.update();
  };

  main("w1");
  

	return 1;
}
