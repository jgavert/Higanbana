// EntryPoint.cpp
#ifdef WIN64
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include "core/src/system/LBS.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/global_debug.hpp"

using namespace faze;

int EntryPoint::main()
{
  Logger log;
  auto main = [&](GraphicsApi api, VendorID preferredVendor, const char* name, bool updateLog)
  {
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
      F_LOG("\t%d. %s (memory: %zd)\n", it.id, it.name.c_str(), it.memory);
    }
    if (updateLog) log.update();
    if (gpus.empty())
      return;
    ivec2 ires = { 800, 600 };
    Window window(m_params, name, ires.x(), ires.y(), 3860, 300);
    window.open();

    FileSystem fs;
    auto surface = graphics.createSurface(window);
    auto dev = graphics.createDevice(fs, gpus[chosenGpu]);
    {
      auto swapchain = dev.createSwapchain(surface);

      F_LOG("Created device \"%s\"\n", gpus[chosenGpu].name.c_str());

      auto bufferdesc = ResourceDescriptor()
        .setName("testBufferTarget")
        .setFormat(FormatType::Float32)
        .setWidth(100)
        .setDimension(FormatDimension::Buffer);

      auto buffer = dev.createBuffer(bufferdesc);

      auto texturedesc = ResourceDescriptor()
        .setName("testTexture")
        .setFormat(FormatType::Unorm8x4_Srgb)
        .setWidth(1280)
        .setHeight(720)
        .setMiplevels(4)
        .setDimension(FormatDimension::Texture2D);

      auto texture = dev.createTexture(texturedesc);

      texturedesc = texturedesc.setArraySize(3);
      auto texture2 = dev.createTexture(texturedesc);

      int64_t frame = 1;
      bool closeAnyway = false;
      while (!window.simpleReadMessages(frame++))
      {
        if (window.hasResized())
        {
          dev.adjustSwapchain(swapchain);
          window.resizeHandled();
        }
        auto& inputs = window.inputs();

        if (inputs.isPressedThisFrame(VK_SPACE, 1))
        {
          auto& mouse = window.mouse();
          F_LOG("%s mouse %d %d\n", name, mouse.m_pos.x(), mouse.m_pos.y());
        }

        if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame(VK_F1, 1))
        {
            window.toggleBorderlessFullscreen();
        }

        if (closeAnyway || inputs.isPressedThisFrame(VK_ESCAPE, 1))
        {
          break;
        }
        if (updateLog) log.update();

      }
    }
  };
  main(GraphicsApi::DX12, VendorID::Nvidia, "Vulkan", true);
 /*
  LBS lbs;
  lbs.addTask("test1", [&](size_t, size_t) {main(GraphicsApi::Vulkan, VendorID::Amd, "Vulkan", true); });
  lbs.addTask("test2", [&](size_t, size_t) {main(GraphicsApi::Vulkan, VendorID::Nvidia, "DX12", false); });
  lbs.sleepTillKeywords({"test1", "test2"});
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
  };
  main("w1");
  t.printStatistics();
  log.update();
  return 0;
}*/