#pragma once
#include "core/src/math/mat_templated.hpp"
//#include "core/src/math/vec_templated.hpp"
#include "Platform/EntryPoint.hpp"
#include "Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/tests/TestWorks.hpp"
#include "core/src/system/time.hpp"
#include "core/src/tools/bentsumaakaa.hpp"
#include "Graphics/gfxApi.hpp"

#include <random>
#include <cstdio>
#include <iostream>

class StressTests
{
private:
  static void runTestsForDevice(GpuDevice& dev, GpuCommandQueue& queue, Window& window, SwapChain& sc, ViewPort& port, GfxCommandList& gfx, faze::Logger& log)
  {
    faze::TestWorks t("advtests");
    t.setAfterTest([&]()
    {
      // clean up
      auto fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait();
      if (!gfx.isClosed())
      {
        gfx.close();
      }
      dev.resetCmdList(gfx);
    });


    t.addTest("Lots of drawcalls bench, (single thread)", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 100000;
      auto currentTriangleCount = triangleCount;
      auto srcdata = dev.createBufferSRV(Dimension(triangleCount), Format<buf>(), ResUsage::Upload);
      auto dstdata = dev.createBufferSRV(Dimension(triangleCount), Format<buf>(), ResUsage::Gpu);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.buffer().Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = dis(gen);
          it[1] = dis(gen);
          it[2] = dis2(gen);
        }
      }

      gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
      GpuFence fence = dev.createFence();
      gfx.close();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel.hlsl")
        .VertexShader("tests/stress/vertex_triangle.hlsl")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      dev.resetCmdList(gfx);
      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;

      //while (frameTime > 9.f)
      while(true)
      {
        if (window.simpleReadMessages())
          break;

        // Rendertarget
        b.start(false);
        gfx.setViewPort(port);
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
        gfx.setRenderTarget(sc[backBufferIndex]);
        // graphics begin
        {
          auto bind = gfx.bind(pipeline);
          bind.SRV(0, dstdata);
          gfx.drawInstanced(bind, 3, 1, 0, 0);
          
          for (int i = 1; i < currentTriangleCount; ++i)
          {
            gfx.drawInstancedRaw(3, 1, 0, i);
          }
          
        }

        // submit all
        gfx.close();
        queue.submit(gfx);
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        dev.resetCmdList(gfx);
        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        if (frameTime > 20.f)
        {
          currentTriangleCount -= currentTriangleCount/100;
        }
        F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
        log.update();
      }
      //fence.wait();
      return false;
    });

    t.addTest("Lots of drawcalls bench, (single thread)", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 100000;
      auto currentTriangleCount = triangleCount;
      auto srcdata = dev.createBufferSRV(Dimension(triangleCount), Format<buf>(), ResUsage::Upload);
      auto dstdata = dev.createBufferSRV(Dimension(triangleCount), Format<buf>(), ResUsage::Gpu);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.buffer().Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = dis(gen);
          it[1] = dis(gen);
          it[2] = dis2(gen);
        }
      }

      gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
      GpuFence fence = dev.createFence();
      gfx.close();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel.hlsl")
        .VertexShader("tests/stress/vertex_triangle.hlsl")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      dev.resetCmdList(gfx);
      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;

      while (cpuTime > 16.f)
      //while (true)
      {
        if (window.simpleReadMessages())
          break;

        // Rendertarget
        b.start(false);
        gfx.setViewPort(port);
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
        gfx.setRenderTarget(sc[backBufferIndex]);
        // graphics begin
        {
          for (int i = 0; i < currentTriangleCount; ++i)
          {
            auto bind = gfx.bind(pipeline);
            bind.SRV(0, dstdata);
            gfx.drawInstanced(bind, 3, 1, 0, i);
          }

        }

        // submit all
        gfx.close();
        queue.submit(gfx);
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        dev.resetCmdList(gfx);
        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        frameTime = t.analyzeFrames().x();
        if (cpuTime > 16.f)
        {
          currentTriangleCount -= currentTriangleCount / 200;
        }
      }
      //fence.wait();
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      return false;
    });

    t.addTest("Lots of drawcalls bench, (single thread)", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 100000;
      auto currentTriangleCount = triangleCount;
      auto srcdata = dev.createBufferSRV(Dimension(triangleCount), Format<buf>(), ResUsage::Upload);
      auto dstdata = dev.createBufferSRV(Dimension(triangleCount), Format<buf>(), ResUsage::Gpu);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.buffer().Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = dis(gen);
          it[1] = dis(gen);
          it[2] = dis2(gen);
        }
      }

      gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
      GpuFence fence = dev.createFence();
      gfx.close();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel.hlsl")
        .VertexShader("tests/stress/vertex_triangle.hlsl")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      dev.resetCmdList(gfx);
      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;

      //while (frameTime > 9.f)
      while (true)
      {
        if (window.simpleReadMessages())
          break;

        // Rendertarget
        b.start(false);
        gfx.setViewPort(port);
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
        gfx.setRenderTarget(sc[backBufferIndex]);
        // graphics begin
        {
          for (int i = 0; i < currentTriangleCount; ++i)
          {
            auto bind = gfx.bind(pipeline);
            bind.SRV(0, dstdata);
            gfx.drawInstanced(bind, 3, 1, 0, i);
          }

        }

        // submit all
        gfx.close();
        queue.submit(gfx);
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        dev.resetCmdList(gfx);
        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        frameTime = t.analyzeFrames().x();
        if (frameTime > 20.f)
        {
          currentTriangleCount -= currentTriangleCount / 100;
        }
        F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
        log.update();
      }
      //fence.wait();
      return false;
    });

    t.runTests();
  }

public:
  static void run(GpuDevice& dev, GpuCommandQueue& queue, Window& window, SwapChain& sc, ViewPort& port, GfxCommandList& gfx, faze::Logger& log)
  {
    runTestsForDevice(dev, queue, window, sc, port, gfx, log);
  }
};