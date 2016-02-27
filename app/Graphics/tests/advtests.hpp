#pragma once
#include "core/src/math/mat_templated.hpp"
//#include "core/src/math/vec_templated.hpp"
#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/tests/TestWorks.hpp"

#include "app/Graphics/gfxApi.hpp"

#include <cstdio>
#include <iostream>

class AdvTests
{
private:
  static void runTestsForDevice(GpuDevice& dev, GpuCommandQueue& queue, Window& window, SwapChain& sc, ViewPort& port, GfxCommandList& gfx)
  {
    faze::TestWorks t("advtests");


    auto end = [&]()
    {
      auto fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait(); // verify that queue has finished with current work
      if (!gfx.isClosed())
      {
        //auto fence = dev.createFence();

        gfx.closeList(); // as expected, explicit close is kind of nice.
        // Much nicer to close and enforce errors if trying to add after that.
        // Although seems like you can still "put" stuff there.
      }
      gfx.resetList();
      // clean the resource use
    };
    //end();
    t.setAfterTest(end);

    t.addTest("Move data to upload heap and move to gpu memory", [&]()
    {
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(4096)
		  .Format<float>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(4096)
		  .Format<float>());
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      {
        auto tmp = srcdata.Map<float>();
        if (!tmp.isValid())
          return false;
        for (auto i = tmp.rangeBegin();i < tmp.rangeEnd(); ++i)
        {
          tmp.get()[i] = static_cast<float>(i);
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      gfx.closeList();
      queue.submit(gfx);
      GpuFence fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait();

      return true;
    });
    t.addTest("Upload and readback the same data", [&]()
    {
      GpuFence fence = dev.createFence();

	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(4096)
		  .Format<float>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(4096)
		  .Format<float>());
	  auto rbdata = dev.createBuffer(ResourceDescriptor()
		  .Width(4096)
		  .Format<float>()
		  .Usage(ResourceUsage::ReadbackHeap));
	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      {
        auto tmp = srcdata.Map<float>();
        for (auto i = tmp.rangeBegin(); i < tmp.rangeEnd(); ++i)
        {
          tmp[i] = static_cast<float>(i);
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      gfx.CopyResource(rbdata, dstdata);
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);
      fence.wait();
      {
        auto woot = rbdata.Map<float>();
        if (!woot.isValid())
          return false;
        for (auto i = woot.rangeBegin(); i < woot.rangeEnd(); ++i)
        {
          if (woot[i] != static_cast<float>(i))
          {
            return false;
          }
        }
        return true;
      }
    });


    t.addTest("Pipeline binding and modify data in compute (sub 50 lines!)", [&]()
    {
      GpuFence fence = dev.createFence();

      ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("compute_1"));

      struct buf
      {
        int i;
        int k;
        int x;
        int y;
      };
      unsigned int shaderGroup = 64;
      unsigned int inputSize = 100;
      unsigned int _bufferSize = shaderGroup*inputSize;

	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(_bufferSize)
		  .Format<float>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(_bufferSize)
		  .Format<float>());
	  auto completedata = dev.createBuffer(ResourceDescriptor()
		  .Width(_bufferSize)
		  .Format<float>()
		  .enableUnorderedAccess());
	  auto rbdata = dev.createBuffer(ResourceDescriptor()
		  .Width(_bufferSize)
		  .Format<float>()
		  .Usage(ResourceUsage::ReadbackHeap));

	  auto dstdataSrv = dev.createBufferSRV(dstdata);
	  auto completedataUav = dev.createBufferUAV(completedata);


      {
        auto tmp = srcdata.Map<buf>();
        for (auto i = tmp.rangeBegin(); i < tmp.rangeEnd(); ++i)
        {
          tmp[i].i = static_cast<int>(i);
          tmp[i].k = static_cast<int>(i);
          tmp[i].x = 0;
          tmp[i].y = 0;
        }
      }

      gfx.CopyResource(dstdata, srcdata);
      auto bind = gfx.bind(pipeline);
      bind.SRV(0, dstdataSrv);
      bind.UAV(0, completedataUav);
      //F_LOG("going over by %u\n", goingOverBy);
      gfx.Dispatch(bind, inputSize, 1, 1);
      /*
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);
      fence.wait();
      gfx.resetList();
      */
      gfx.CopyResource(rbdata, completedata);
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);
      fence.wait();
      {
        auto mapd = rbdata.Map<buf>();
        if (!mapd.isValid())
          return false;
        for (auto i = mapd.rangeBegin(); i < mapd.rangeEnd(); ++i)
        {
          auto& obj = mapd[i];
          if (obj.i != i + i)
          {
            return false;
          }
          if (obj.k != i)
          {
            return false;
          }
        }
      }
      return true;
    });


    t.addTest("render(?) for full 1 second in loop", [&]()
    {
      auto timepoint2 = std::chrono::high_resolution_clock::now();
      float i = 0;
      auto vec = faze::vec4({ i, 0.2f, 0.2f, 1.0f });
      auto limit = std::chrono::seconds(1).count();

      auto timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
      while (timeSince < limit)
      {
        timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
        window.simpleReadMessages();
        GpuFence fence = dev.createFence();
        gfx.setViewPort(port);
        auto backBufferIndex = sc.GetCurrentBackBufferIndex();
        vec[0] += 0.02f;
        if (vec[0] > 1.0f)
          vec[0] = 0.f;
        gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
        gfx.closeList();
        queue.submit(gfx);

        sc.present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        gfx.resetList();

      }
      return true;
    });

    t.addTest("render a triangle for full 1 second in loop", [&]()
    {
      struct buf
      {
        float pos[4];
      };
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(6)
		  .Format<float>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(6)
		  .Format<float>());

	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      {
        auto tmp = srcdata.Map<buf>();
        float size = 0.8f;
        tmp[0] = { size, size, -0.1f, 1.f };
        tmp[1] = { size, -size, 1.1f, 1.f };
        tmp[2] = { -size, size, 1.1f, 1.f };
        tmp[3] = { -size, size, 0.0f, 1.f };
        tmp[4] = { size, -size, 0.0f, 1.f };
        tmp[5] = { -size, -size, 0.2f, 1.f };
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("pixel")
        .VertexShader("vertex")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      auto limit = std::chrono::seconds(1).count();
      auto timepoint2 = std::chrono::high_resolution_clock::now();
      auto timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
      while (timeSince < limit)
      {
        timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
        if (window.simpleReadMessages())
          break;
        fence.wait();
        gfx.resetList();


        // Rendertarget
        gfx.setViewPort(port);
        vec[2] += 0.02f;
        if (vec[2] > 1.0f)
          vec[2] = 0.f;
        auto backBufferIndex = sc.GetCurrentBackBufferIndex();
        gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
        gfx.setRenderTarget(sc[backBufferIndex]);
        // graphics begin
        auto bind = gfx.bind(pipeline);
        bind.SRV(0, dstdataSrv);
        gfx.drawInstanced(bind, 6,1,0,0);


        // submit all
        gfx.preparePresent(sc[backBufferIndex]);
        gfx.closeList();
        queue.submit(gfx);

        // present
        sc.present(1, 0);
        queue.insertFence(fence);
      }
      fence.wait();

      return true;
    });

    t.addTest("Render triangle with perspective", [&]()
    {
      using namespace faze;
      struct ConstantsCustom
      {
        mat4 WorldMatrix;
        mat4 ViewMatrix;
        mat4 ProjectionMatrix;
        float time;
        vec2 resolution;
        float filler;
      };

	  auto srcConstants = dev.createBuffer(ResourceDescriptor()
		  .Format<ConstantsCustom>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstConstants = dev.createBuffer(ResourceDescriptor()
		  .Format<ConstantsCustom>());

	  auto dstConstantsCbv = dev.createBufferCBV(dstConstants);


      gfx.CopyResource(dstConstants, srcConstants);

      struct buf
      {
        float pos[4];
	  };
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(6)
		  .Format<float>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(6)
		  .Format<float>());

	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      {
        auto tmp = srcdata.Map<buf>();
        float size = 0.5f;
        float z = 0.f;
        tmp[0] = { 0, size, z - 0.01f, 1.f };
        tmp[1] = { size, -size, z - 0.01f, 1.f };
        tmp[2] = { -size, -size, z-0.01f, 1.f };
        tmp[3] = { -size, -size, z+0.01f, 1.f };
        tmp[4] = { size, -size, z+0.01f, 1.f };
        tmp[5] = { 0,size, z+0.01f, 1.f };
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto tex = dev.createTexture(ResourceDescriptor()
        .Width(800).Height(600)
        .Format(FormatType::D32_FLOAT)
        .enableDepthStencil());
      auto depth = dev.createTextureDSV(tex);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("pixel")
        .VertexShader("vertex2")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
        .DSVFormat(FormatType::D32_FLOAT)
        .DepthStencil(DepthStencilDescriptor()
          .DepthEnable(true)
          .DepthWriteMask(DepthWriteMask::All)
          .DepthFunc(ComparisonFunc::LessEqual))
        );

      auto vec = faze::vec4({ 0.1f, 0.1f, 0.3f, 1.0f });

      auto limit = std::chrono::seconds(1).count();
      auto timepoint2 = std::chrono::high_resolution_clock::now();
      auto timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
      while (timeSince < limit)
      //while (true)
      {
        timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
        auto timeSince2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
        if (window.simpleReadMessages())
          break;
        fence.wait();
        gfx.resetList();


        // yay, update the constant buffer for the frame
        {
          auto tmp = srcConstants.Map<ConstantsCustom>();
          tmp[0].WorldMatrix = MatrixMath::Translation(std::sin(timeSince2*0.001f), std::cos(timeSince2*0.001f), 1.f);
          // look at target 'z' is negative !?
          tmp[0].ViewMatrix = MatrixMath::lookAt(vec4({ 0.f, 0.0f, 0.0f, 1.f }), vec4({ std::sin(timeSince2*0.001f),std::cos(timeSince2*0.001f), -1.0f, 1.f }));
          tmp[0].ProjectionMatrix = MatrixMath::Perspective(60.f, 800.f / 600.f, 0.01f, 100.f);
          tmp[0].time = 0.f;
          tmp[0].resolution = { 800.f, 600.f };
          tmp[0].filler = 0.f;
        }
        gfx.CopyResource(dstConstants, srcConstants);
        // Rendertarget
        gfx.setViewPort(port);
        auto backBufferIndex = sc.GetCurrentBackBufferIndex();
        gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
        gfx.ClearDepthView(depth);
        gfx.setRenderTarget(sc[backBufferIndex], depth);
        // graphics begin
        auto bind = gfx.bind(pipeline);
        bind.SRV(0, dstdataSrv);
        bind.CBV(0, dstConstantsCbv);
        gfx.drawInstanced(bind, 6, 1, 0, 0);


        // submit all
        gfx.preparePresent(sc[backBufferIndex]);
        gfx.closeList();
        queue.submit(gfx);

        // present
        sc.present(1, 0);
        queue.insertFence(fence);
      }
      fence.wait();

      return true;
    });

    t.runTests();
  }

public:
  static void run(GpuDevice& dev, GpuCommandQueue& queue, Window& window, SwapChain& sc, ViewPort& port, GfxCommandList& gfx)
  {
    runTestsForDevice(dev, queue, window, sc, port, gfx);
  }
};
