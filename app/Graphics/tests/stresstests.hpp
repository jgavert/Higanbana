#pragma once
#include "core/src/math/mat_templated.hpp"
//#include "core/src/math/vec_templated.hpp"
#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/tests/TestWorks.hpp"
#include "core/src/system/time.hpp"
#include "core/src/tools/bentsumaakaa.hpp"
#include "app/Graphics/gfxApi.hpp"

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
        gfx.closeList();
      }
      gfx.resetList();

    });

    
    t.addTest("Lots of drawcalls bench, (single thread), rough baseline", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 1000000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
      auto dstdataSrv = dev.createBufferSRV(dstdata);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      gfx.resetList();

      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;

      while (cpuTime > 14.f)
      //while(true)
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
          bind.SRV(0, dstdataSrv);
          gfx.drawInstanced(bind, 3, 1, 0, 0);
          
          for (int i = 1; i < currentTriangleCount; ++i)
          {
            gfx.drawInstancedRaw(3, 1, 0, i); // this is the cheat.
          }
          
        }

        // submit all
        gfx.closeList();
        queue.submit(gfx);
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        gfx.resetList();

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        if (cpuTime > 16.f)
        {
          currentTriangleCount -= currentTriangleCount/100;
        }
      }
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      //fence.wait();
      return true;
    });

    t.addTest("Lots of drawcalls bench, (single thread), myapi", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 200000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);


      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      gfx.resetList();

      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;

      while (cpuTime > 14.f)
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
            bind.SRV(0, dstdataSrv);
            gfx.drawInstanced(bind, 3, 1, 0, i);
          }
        }

        // submit all
        gfx.closeList();
        queue.submit(gfx);
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        gfx.resetList();

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        frameTime = t.analyzeFrames().x();
        if (cpuTime > 16.f)
        {
          currentTriangleCount -= currentTriangleCount / 100;
        }
      }
      //fence.wait();
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      return true;
    });

    t.addTest("Lots of drawcalls bench, (Multithread), baseline", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 4000000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      gfx.resetList();

      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;
      std::vector<GfxCommandList> m_cmds;
      for (size_t i = 0; i < lbs.threadCount(); ++i)
      {
        m_cmds.push_back(dev.createUniversalCommandList());
      }
      //while (true)
      while (cpuTime > 14.f)
      {
        if (window.simpleReadMessages())
          break;

        // Rendertarget
        b.start(false);
        m_cmds[0].setViewPort(port);
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        m_cmds[0].ClearRenderTargetView(sc[backBufferIndex], vec);
        m_cmds[0].setRenderTarget(sc[backBufferIndex]);
        for (size_t i = 1; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].setViewPort(port);
          //m_cmds[i].ClearRenderTargetView(sc[backBufferIndex], vec);
          m_cmds[i].setRenderTarget(sc[backBufferIndex]);
        }
        // graphics begin
        lbs.addParallelFor<1>("fillCommands", {}, {}, 0, 100, [&](size_t id, size_t threadIndex)
        {
          auto& gfx2 = m_cmds[threadIndex];
          unsigned workAmount = currentTriangleCount / 100;
          unsigned startIndex = static_cast<unsigned>(workAmount * id);
          auto bind = gfx2.bind(pipeline);
          bind.SRV(0, dstdataSrv);
          gfx2.drawInstanced(bind, 3, 1, 0, startIndex);
          for (unsigned i = startIndex+1; i < startIndex + workAmount; ++i)
          {
            gfx2.drawInstancedRaw(3, 1, 0, i);
          }
        });
        lbs.sleepTillKeywords({ "fillCommands" });
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].closeList();
          queue.submit(m_cmds[i]);
        }
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].resetList();
        }

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        frameTime = t.analyzeFrames().x();
        if (cpuTime > 16.f)
        {
          currentTriangleCount -= currentTriangleCount / 100;
        }
      }
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      return true;
    });

    t.addTest("Lots of drawcalls bench, (Multithread), myapi", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 2000000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      gfx.resetList();

      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;
      std::vector<GfxCommandList> m_cmds;
      for (size_t i = 0; i < lbs.threadCount(); ++i)
      {
        m_cmds.push_back(dev.createUniversalCommandList());
      }
      //while (true)
      while (cpuTime > 14.f)
      {
        if (window.simpleReadMessages())
          break;

        // Rendertarget
        b.start(false);
        m_cmds[0].setViewPort(port);
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        m_cmds[0].ClearRenderTargetView(sc[backBufferIndex], vec);
        m_cmds[0].setRenderTarget(sc[backBufferIndex]);
        for (size_t i = 1; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].setViewPort(port);
          //m_cmds[i].ClearRenderTargetView(sc[backBufferIndex], vec);
          m_cmds[i].setRenderTarget(sc[backBufferIndex]);
        }
        // graphics begin
        lbs.addParallelFor<1>("fillCommands", {}, {}, 0, 100, [&](size_t id, size_t threadIndex)
        {
          auto& gfx2 = m_cmds[threadIndex];
          unsigned workAmount = currentTriangleCount / 100;
          unsigned startIndex = static_cast<unsigned>(workAmount * id);
          for (unsigned i = startIndex; i < startIndex + workAmount; ++i)
          {
            auto bind = gfx2.bind(pipeline);
            bind.SRV(0, dstdataSrv);
            gfx2.drawInstanced(bind, 3, 1, 0, i);
          }
        });
        lbs.sleepTillKeywords({ "fillCommands" });
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].closeList();
          queue.submit(m_cmds[i]);
        }
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].resetList();
        }

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        frameTime = t.analyzeFrames().x();
        if (cpuTime > 16.f)
        {
          currentTriangleCount -= currentTriangleCount / 40;
        }
      }
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      return true;
    });

    t.addTest("Lots of drawcalls bench, (single thread), rough baseline, frametime 20ms", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 800000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);
      fence.wait();
      gfx.resetList();

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });


      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;

      int beenUnderLimit = 0;
      while (beenUnderLimit < 10)
      {
        if (frameTime < 20.f)
          beenUnderLimit++;
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
          bind.SRV(0, dstdataSrv);
          gfx.drawInstanced(bind, 3, 1, 0, 0);

          for (int i = 1; i < currentTriangleCount; ++i)
          {
            gfx.drawInstancedRaw(3, 1, 0, i); // this is the cheat.
          }

        }

        // submit all
        gfx.closeList();
        queue.submit(gfx);
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        gfx.resetList();

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        if (frameTime > 20.f)
        {
          currentTriangleCount -= currentTriangleCount / 100;
        }
      }
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      //fence.wait();
      return true;
    });

    t.addTest("Lots of drawcalls bench, (single thread), myapi, frametime 20ms", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 100000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      gfx.resetList();

      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;

      int beenUnderLimit = 0;
      while (beenUnderLimit < 10)
      {
        if (frameTime < 20.f)
          beenUnderLimit++;
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
            bind.SRV(0, dstdataSrv);
            gfx.drawInstanced(bind, 3, 1, 0, i);
          }
        }

        // submit all
        gfx.closeList();
        queue.submit(gfx);
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        gfx.resetList();

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        //frameTime = t.analyzeFrames().x();
        if (frameTime > 20.f)
        {
          currentTriangleCount -= currentTriangleCount / 100;
        }
      }
      //fence.wait();
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      return true;
    });

    t.addTest("Lots of drawcalls bench, (Multithread), baseline, frametime 20ms", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 1000000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      gfx.resetList();

      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;
      std::vector<GfxCommandList> m_cmds;
      for (size_t i = 0; i < lbs.threadCount(); ++i)
      {
        m_cmds.push_back(dev.createUniversalCommandList());
      }
      //while (true)
      int beenUnderLimit = 0;
      while (beenUnderLimit < 10)
      {
        if (frameTime < 20.f)
          beenUnderLimit++;
        if (window.simpleReadMessages())
          break;

        // Rendertarget
        b.start(false);
        m_cmds[0].setViewPort(port);
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        m_cmds[0].ClearRenderTargetView(sc[backBufferIndex], vec);
        m_cmds[0].setRenderTarget(sc[backBufferIndex]);
        for (size_t i = 1; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].setViewPort(port);
          //m_cmds[i].ClearRenderTargetView(sc[backBufferIndex], vec);
          m_cmds[i].setRenderTarget(sc[backBufferIndex]);
        }
        // graphics begin
        lbs.addParallelFor<1>("fillCommands", {}, {}, 0, 100, [&](size_t id, size_t threadIndex)
        {
          auto& gfx2 = m_cmds[threadIndex];
          unsigned workAmount = static_cast<unsigned>(currentTriangleCount / 100);
          unsigned startIndex = static_cast<unsigned>(workAmount * id);
          auto bind = gfx2.bind(pipeline);
          bind.SRV(0, dstdataSrv);
          gfx2.drawInstanced(bind, 3, 1, 0, startIndex);
          for (unsigned i = startIndex + 1; i < startIndex + workAmount; ++i)
          {
            gfx2.drawInstancedRaw(3, 1, 0, i);
          }
        });
        lbs.sleepTillKeywords({ "fillCommands" });
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].closeList();
          queue.submit(m_cmds[i]);
        }
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].resetList();
        }

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        //frameTime = t.analyzeFrames().x();
        if (frameTime > 20.f)
        {
          currentTriangleCount -= currentTriangleCount / 120;
        }
      }
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      return true;
    });

    t.addTest("Lots of drawcalls bench, (Multithread), myapi, frametime 20ms", [&]()
    {
      using namespace faze;
      struct buf
      {
        float pos[4];
      };
      auto triangleCount = 400000;
      auto currentTriangleCount = triangleCount;
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(triangleCount)
		  .Format<buf>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(-0.8f, 0.8f);
      std::uniform_real_distribution<> dis2(0.f, 1.f);

      {
        auto tmp = srcdata.Map<buf>();
        for (int i = 0;i < triangleCount; ++i)
        {
          auto& it = tmp[i].pos;
          it[0] = static_cast<float>(dis(gen));
          it[1] = static_cast<float>(dis(gen));
          it[2] = static_cast<float>(dis2(gen));
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("tests/stress/pixel")
        .VertexShader("tests/stress/vertex_triangle")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      fence.wait();
      gfx.resetList();

      WTime t;
      t.firstTick();
      float frameTime = 30.f;
      float cpuTime = 30.f;
      Bentsumaakaa b;
      LBS lbs;
      std::vector<GfxCommandList> m_cmds;
      for (size_t i = 0; i < lbs.threadCount(); ++i)
      {
        m_cmds.push_back(dev.createUniversalCommandList());
      }
      //while (true)
      int beenUnderLimit = 0;
      while (beenUnderLimit < 10)
      {
        if (frameTime < 20.f)
          beenUnderLimit++;
        if (window.simpleReadMessages())
          break;

        // Rendertarget
        b.start(false);
        m_cmds[0].setViewPort(port);
        auto backBufferIndex = sc->GetCurrentBackBufferIndex();
        m_cmds[0].ClearRenderTargetView(sc[backBufferIndex], vec);
        m_cmds[0].setRenderTarget(sc[backBufferIndex]);
        for (size_t i = 1; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].setViewPort(port);
          //m_cmds[i].ClearRenderTargetView(sc[backBufferIndex], vec);
          m_cmds[i].setRenderTarget(sc[backBufferIndex]);
        }
        // graphics begin
        lbs.addParallelFor<1>("fillCommands", {}, {}, 0, 100, [&](size_t id, size_t threadIndex)
        {
          auto& gfx2 = m_cmds[threadIndex];
          size_t workAmount = currentTriangleCount / 100;
          size_t startIndex = workAmount * id;
          for (unsigned i = static_cast<unsigned>(startIndex); i < static_cast<unsigned>(startIndex + workAmount); ++i)
          {
            auto bind = gfx2.bind(pipeline);
            bind.SRV(0, dstdataSrv);
            gfx2.drawInstanced(bind, 3, 1, 0, i);
          }
        });
        lbs.sleepTillKeywords({ "fillCommands" });
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].closeList();
          queue.submit(m_cmds[i]);
        }
        cpuTime = b.stop(false) / 1000000.f;
        // present
        sc->Present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        for (size_t i = 0; i < lbs.threadCount(); ++i)
        {
          m_cmds[i].resetList();
        }

        t.tick();
        frameTime = static_cast<float>(t.getCurrentNano())*0.000001f;
        //frameTime = t.analyzeFrames().x();
        if (frameTime > 20.f)
        {
          currentTriangleCount -= currentTriangleCount / 120;
        }
      }
      F_LOG("frametime: %.3fms CpuTime: %.3fms TriangleCount: %d\n", frameTime, cpuTime, currentTriangleCount);
      log.update();
      return true;
    });

    t.runTests();
  }

public:
  static void run(GpuDevice& dev, GpuCommandQueue& queue, Window& window, SwapChain& sc, ViewPort& port, GfxCommandList& gfx, faze::Logger& log)
  {
    runTestsForDevice(dev, queue, window, sc, port, gfx, log);
  }
};