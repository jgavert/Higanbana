#include "core/src/Platform/EntryPoint.hpp"

// EntryPoint.cpp
#ifdef FAZE_PLATFORM_WINDOWS
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include "core/src/neural/network.hpp"
#include "core/src/system/LBS.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/global_debug.hpp"

#include "core/src/math/vec_templated.hpp"
#include "shaders/triangle.if.hpp"

#include "faze/src/new_gfx/vk/util/ShaderStorage.hpp"
#include "faze/src/new_gfx/dx12/util/ShaderStorage.hpp"

using namespace faze;

int EntryPoint::main()
{
  Logger log;
  {
    //testNetwork(log);
    //log.update();
  }

  auto main = [&](GraphicsApi api, VendorID preferredVendor, const char* name, bool updateLog)
  {
    bool reInit = false;
    int64_t frame = 1;
    FileSystem fs;

    backend::DX12ShaderStorage shadersdx(fs, "../app/shaders", "dxil");
    auto res = shadersdx.compileShader("triangle", backend::DX12ShaderStorage::ShaderType::Vertex);
    F_LOG("compiled! %d\n", int(res));

    ShaderStorage shaders(fs, "../app/shaders", "spirv");
    res = shaders.compileShader("triangle", ShaderStorage::ShaderType::Vertex);
    F_LOG("compiled! %d\n", int(res));

    while (true)
    {
      ivec2 ires = { 800, 600 };
      Window window(m_params, name, ires.x(), ires.y(), 300, 200);
      window.open();
      GraphicsSubsystem graphics(api, name);
      F_LOG("Using api %s\n", graphics.gfxApi().c_str());
      F_LOG("Have gpu's\n");
      auto gpus = graphics.availableGpus();
      int chosenGpu = 0;
      for (auto&& it : gpus)
      {
        if (it.vendor == preferredVendor)
        {
          chosenGpu = it.id;
        }
        F_LOG("\t%d. %s (memory: %zd, api: %s)\n", it.id, it.name.c_str(), it.memory, it.apiVersionStr.c_str());
      }
      if (updateLog) log.update();
      if (gpus.empty())
        return;

      auto surface = graphics.createSurface(window);
      auto dev = graphics.createDevice(fs, gpus[chosenGpu]);
      {
        auto swapchain = dev.createSwapchain(surface);

        F_LOG("Created device \"%s\"\n", gpus[chosenGpu].name.c_str());

        decltype(::shader::Triangle::constants) constants{};
        constants.color = float4{ 1.f, 0.f, 0.f, 1.f };

        auto bufferdesc = ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat(FormatType::Float32)
          .setWidth(100)
          .setDimension(FormatDimension::Buffer);

        auto buffer = dev.createBuffer(bufferdesc);

        auto texture = dev.createTexture(ResourceDescriptor()
          .setName("testTexture")
          .setFormat(FormatType::Unorm8x4_Srgb)
          .setWidth(ires.x())
          .setHeight(ires.y())
          .setMiplevels(4)
          .setDimension(FormatDimension::Texture2D)
          .setUsage(ResourceUsage::RenderTarget));
        auto texRtv = dev.createTextureRTV(texture, ShaderViewDescriptor().setMostDetailedMip(0));
        auto texSrv = dev.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));

        Renderpass triangleRenderpass = dev.createRenderpass();

        auto pipelineDescriptor = GraphicsPipelineDescriptor()
          .setVertexShader("triangle")
          .setPixelShader("triangle")
          .setPrimitiveTopology(PrimitiveTopology::Triangle)
		  //.setRasterizer(faze::RasterizerDescriptor()
		  //	.setFrontCounterClockwise(true))
          .setDepthStencil(DepthStencilDescriptor()
            .setDepthEnable(false));

        GraphicsPipeline trianglePipe = dev.createGraphicsPipeline(pipelineDescriptor);
        F_LOG("%d\n", trianglePipe.descriptor.sampleCount);

        bool closeAnyway = false;
        while (!window.simpleReadMessages(frame++))
        {
          if (window.hasResized())
          {
            dev.adjustSwapchain(swapchain);
            window.resizeHandled();

            auto& desc = swapchain.buffers()[0].desc().desc;

            texture = dev.createTexture(ResourceDescriptor()
              .setName("testTexture")
              .setFormat(FormatType::Unorm8x4_Srgb)
              .setWidth(desc.width)
              .setHeight(desc.height)
              .setMiplevels(4)
              .setDimension(FormatDimension::Texture2D)
              .setUsage(ResourceUsage::RenderTarget));
            texRtv = dev.createTextureRTV(texture, ShaderViewDescriptor().setMostDetailedMip(0));
            texSrv = dev.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
          }
          auto& inputs = window.inputs();

          if (inputs.isPressedThisFrame(VK_SPACE, 1))
          {
            auto& mouse = window.mouse();
            F_LOG("%s mouse %d %d\n", name, mouse.m_pos.x(), mouse.m_pos.y());
          }

          if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('1', 1))
          {
            window.toggleBorderlessFullscreen();
          }

          if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('2', 1))
          {
            reInit = true;
            F_LOG("switch api\n");
            break;
          }

          if (frame > 10 && (closeAnyway || inputs.isPressedThisFrame(VK_ESCAPE, 1)))
          {
            break;
          }
          if (updateLog) log.update();

          // If you acquire, you must submit it. Next, try to first present empty image.
          // On vulkan, need to at least clear the image or we will just get error about it. (... well at least when the contents are invalid in the beginning.)
          auto backbuffer = dev.acquirePresentableImage(swapchain);
          CommandGraph tasks = dev.createGraph();
          {
            auto node = tasks.createPass("clear");
            node.clearRT(backbuffer, vec4{ std::sin(float(frame)*0.01f)*.5f + .5f, 0.f, 0.f, 1.f });
            node.clearRT(texRtv, vec4{ 0.f, std::sin(float(frame)*0.01f)*.5f + .5f, 0.f, 1.f });
            tasks.addPass(std::move(node));
          }
		  if (inputs.isPressedThisFrame('3', 2))
		  {
			// we have pulsing red color background, draw a triangle on top of it !
            auto node = tasks.createPass("Triangle!");
            node.renderpass(triangleRenderpass);
            node.subpass(backbuffer);
            auto binding = node.bind<::shader::Triangle>(trianglePipe);
            binding.constants.color = float4{ 0.f, 1.f, 0.f, 1.f };
            node.draw(binding, 3, 1);
            node.endRenderpass();
            tasks.addPass(std::move(node));
          }
          {
            auto& node = tasks.createPass2("present");
            node.prepareForPresent(backbuffer);
          }

          dev.submit(swapchain, tasks);

          dev.present(swapchain);
        }
      }
      if (!reInit)
        break;
      else
      {
        reInit = false;
        if (api == GraphicsApi::Vulkan)
        {
          api = GraphicsApi::DX12;
        }
        else
        {
          api = GraphicsApi::Vulkan;
        }
      }
    }
  };
  main(GraphicsApi::DX12, VendorID::Amd, "amd", true);
  /*
   LBS lbs;
   lbs.addTask("test1", [&](size_t, size_t) {main(GraphicsApi::DX12, VendorID::Nvidia, "Vulkan", true); });
   //lbs.addTask("test2", [&](size_t, size_t) {main(GraphicsApi::Vulkan, VendorID::Nvidia, "DX12", false); });
   lbs.sleepTillKeywords({"test1"});
   */
  log.update();
  return 1;
}
/*
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
>>>>>>> ce3488383bf67f16ae039a26f9602cdaea77af61

#include "core/src/system/PageAllocator.hpp"

#include "core/src/system/memview.hpp"
#include "core/src/spirvcross/spirv_glsl.hpp"

#include "core/src/tools/renderdoc.hpp"

#include "vkShaders/sampleShader.if.hpp"

#include <shaderc/shaderc.hpp>
#include <cstdio>
#include <iostream>
#include <sparsepp.h>

using namespace faze;

// thinking of gpu, 1024*64 is the minimum size
// wait... this works by (I cannot allocate 64kb -> allocate 64^2...? otherwise 64->128?
// this split can also be done up right.

void testAlloca()
{
  //SegregatedBlockAllocator a(6, 12, 8);
  PageAllocator a(64, 8*64);

  auto block = a.allocate(64 * 64 * 7);
  a.release(block);
  auto block2a = a.allocate(64 * 64 * 4);
  auto block2b = a.allocate(64 * 64 * 2); // somewhere in middle
  auto block3 = a.allocate(64 * 64 * 7);
  F_ASSERT(block3.offset == -1, "Should fail. Not succeed.");
  a.release(block2a);
  auto block3a = a.allocate(64 * 64 * 7);
  F_ASSERT(block3a.offset == -1, "Should fail. Not succeed.");
  a.release(block2b);
  auto block3b = a.allocate(64 * 64 * 7);
  F_ASSERT(block3b.offset != -1, "Should succeed.");
  F_LOG("test %d\n", block3b.size);
  a.release(block3b);
}

int EntryPoint::main()
{
  WTime t;
  t.firstTick();
  Logger log;
  testAlloca();
  log.update();
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
  RenderDocApi renderdoc;
  auto main = [&](std::string name)
  {
<<<<<<< HEAD
    testNetwork(log);
    log.update();
=======
    //LBS lbs;
    ivec2 ires = { 800, 600 };
    vec2 res = { static_cast<float>(ires.x()), static_cast<float>(ires.y()) };
    Window window(m_params, name, ires.x(), ires.y());
    window.open();
    int64_t frame = 1;
    {
      GpuDevice gpu = devices.createGpuDevice(fs);
      WindowSurface surface = devices.createSurface(window);
      auto swapchain = gpu.createSwapchain(surface, PresentMode::Mailbox);

      // TODO1: advance towards usable images, somewhat done
      // TODO2: cleanup texture format, need formats, bytes per pixel, compressed information. for only the formats that are used.

      //renderdoc.startCapture();
      {
        constexpr int TestBufferSize = 1 * 128;

        constexpr size_t Memsize = 64 * 1024 * 64;

        auto testHeap = gpu.createMemoryHeap(HeapDescriptor()
          .setName("ebin")
          .sizeInBytes(Memsize)
          .setHeapType(HeapType::Upload)); // 32megs, should be the common size...
        auto testHeap2 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinTarget").sizeInBytes(Memsize).setHeapType(HeapType::Default)); // 32megs, should be the common size...
        auto testHeap3 = gpu.createMemoryHeap(HeapDescriptor().setName("ebinReadback").sizeInBytes(Memsize).setHeapType(HeapType::Readback)); // 32megs, should be the common size...
        auto buffer = gpu.createBuffer(testHeap,
          ResourceDescriptor()
          .setName("testBuffer")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .setUsage(ResourceUsage::UploadHeap)
          .setDimension(FormatDimension::Buffer));

        auto bufferTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .enableUnorderedAccess()
          .setUsage(ResourceUsage::GpuOnly)
          .setDimension(FormatDimension::Buffer));
        auto bufferTargetUav = gpu.createBufferUAV(bufferTarget);
        auto computeTarget = gpu.createBuffer(testHeap2, // bind memory fails?
          ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .setUsage(ResourceUsage::GpuOnly)
          .enableUnorderedAccess()
          .setDimension(FormatDimension::Buffer));
        auto computeTargetUav = gpu.createBufferUAV(computeTarget);
        auto bufferReadb = gpu.createBuffer(testHeap3,
          ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat<float>()
          .setWidth(TestBufferSize)
          .setUsage(ResourceUsage::ReadbackHeap)
          .setDimension(FormatDimension::Buffer));

        auto testTexture = gpu.createTexture(testHeap2, ResourceDescriptor()
          .setName("TestTexture")
          .setWidth(800)
          .setHeight(600)
          .setFormat(FormatType::R8G8B8A8_Uint)
          .setDimension(FormatDimension::Texture2D)
          .setUsage(ResourceUsage::GpuOnly)
          .setLayout(TextureLayout::StandardSwizzle64kb));

        if (buffer.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = buffer.Map<float>(0, TestBufferSize);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer!\n");
              for (auto i = 0; i < TestBufferSize; ++i)
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
            gfx.dispatchThreads(shif, TestBufferSize);
          }
          gpu.submit(gfx);
        }

        t.firstTick();

        float value = 0.f;

        Renderpass rp = gpu.createRenderpass();

        while (!window.simpleReadMessages(frame++))
        {
          if (window.hasResized())
          {
            gpu.reCreateSwapchain(swapchain, surface);
            window.resizeHandled();
          }

          auto& inputs = window.inputs();

          if (inputs.isPressedThisFrame(VK_SPACE, 1))
          {
            auto& mouse = window.mouse();
            F_LOG("mouse %d %d\n", mouse.m_pos.x(), mouse.m_pos.y());
          }

          if (inputs.isPressedThisFrame(VK_ESCAPE, 1))
          {
            // \o/ is work
            // mouse next, input capture in window?
            break;
          }

          log.update();

          {
            auto rtv = gpu.acquirePresentableImage(swapchain);
            {
              auto gfx = gpu.createGraphicsCommandBuffer();
              gfx.clearRTV(rtv, value);
              {
                auto insiderp = gfx.renderpass(rp, rtv);
              }
              gpu.submitSwapchain(gfx, swapchain);
            }
            gpu.present(swapchain);
            t.tick();
            value += t.getFrameTimeDelta();
            if (value > 1.f)
              value = 0.f;
          }
        }
        // outside windowloop

        {
          auto gfx = gpu.createGraphicsCommandBuffer();
          auto shif = gfx.bind<SampleShader>(test);
          for (int k = 0; k < 1; k++)
          {
            {
              shif.read(SampleShader::dataIn, computeTargetUav);
              shif.modify(SampleShader::dataOut, bufferTargetUav);
              gfx.dispatchThreads(shif, TestBufferSize);
            }
            {
              shif.read(SampleShader::dataIn, bufferTargetUav);
              shif.modify(SampleShader::dataOut, computeTargetUav);
              gfx.dispatchThreads(shif, TestBufferSize);
            }
          }
          gpu.submit(gfx);
        }

        {
          auto gfx = gpu.createGraphicsCommandBuffer();
          gfx.copy(computeTarget, bufferReadb);
          gpu.submit(gfx);
        }
        gpu.waitIdle();

        if (bufferReadb.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = bufferReadb.Map<float>(0, TestBufferSize);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer! %f\n", map[TestBufferSize - 1]);
              log.update();
            }
          }
        }
        //renderdoc.endCapture();
      }
    }
>>>>>>> ce3488383bf67f16ae039a26f9602cdaea77af61
  };
  main("w1");
  t.printStatistics();
  log.update();
  return 0;
}*/