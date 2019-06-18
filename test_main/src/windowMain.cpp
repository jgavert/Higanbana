#include "windowMain.hpp"
#include "core/system/LBS.hpp"
#include "graphics/GraphicsCore.hpp"
#include "core/filesystem/filesystem.hpp"
#include "core/Platform/Window.hpp"
#include "core/system/logger.hpp"
#include "core/global_debug.hpp"
#include <core/entity/database.hpp>
#include <core/entity/query.hpp>

#include <tuple>

#include <core/system/misc.hpp>

using namespace faze;
using namespace faze::math;
/*
void printRange(const SubresourceRange& range)
{
  F_LOG("(%d, %d)(%d, %d)\n", range.mipOffset, range.mipLevels, range.sliceOffset, range.arraySize);
}*/

#include "graphics/desc/shader_input_descriptor.hpp"

STRUCT_DECL(PixelConstants,
  float resx;
  float resy;
  float time;
  int unused;
);

void mainWindow(ProgramParams& params)
{
  Logger log;

  /*
  Database<10> db;
  auto& ft = db.get<float>();
  auto& f4t = db.get<float4>();

  auto e = db.createEntity();
  ft.get(e) = 4.f;
  f4t.get(e) = float4(1.f, 2.f, 3.f, 4.f);

  query(pack(ft, f4t), [](float& a, float4& vec)
  {
    F_ILOG("query", "super values a: %.2f f4:%.2f %.2f %.2f %.2f", a, vec.x, vec.y, vec.z, vec.w);
  });
  */


  //LBS lbs;

  auto main = [&](GraphicsApi api, VendorID preferredVendor, bool updateLog)
  {
    bool reInit = false;
    int64_t frame = 1;
    FileSystem fs("/../../data");
    bool quit = false;

    log.update();

    bool explicitID = false;
    //GpuInfo gpuinfo{};
    vector<GpuInfo> allGpus;

    while (true)
    {
      GraphicsSubsystem graphics("faze");
      F_LOG("Have gpu's\n");
      auto gpus = graphics.availableGpus();
      if (!explicitID)
      {
        //gpuinfo = graphics.getVendorDevice(api);
        for (auto&& it : gpus)
        {
          if (it.api == api && it.vendor == preferredVendor)
          {
            allGpus.push_back(it);
            break;
          }
          F_LOG("\t%s: %d. %s (memory: %zdMB, api: %s)\n", toString(it.api), it.id, it.name.c_str(), it.memory/1024/1024, it.apiVersionStr.c_str());
        }
      }
      if (updateLog) log.update();
      if (gpus.empty())
        return;

	    std::string windowTitle = "";
      for (auto& gpu : allGpus)
      {
        windowTitle += std::string(toString(gpu.api)) + ": " + gpu.name + " ";
      }
      Window window(params, windowTitle, 1280, 720, 300, 200);
      window.open();

      auto surface = graphics.createSurface(window, allGpus[0]);
      auto dev = graphics.createGroup(fs, allGpus);
      {
        auto toggleHDR = false;
        auto scdesc = SwapchainDescriptor()
          .formatType(FormatType::Unorm8RGBA)
          .colorspace(Colorspace::BT709)
          .bufferCount(3).presentMode(PresentMode::Fifo);
        auto swapchain = dev.createSwapchain(surface, scdesc);

        for (auto& gpu : allGpus)
        {
          F_LOG("Created device \"%s\"\n", gpu.name.c_str());
        }

        auto bufferdesc = ResourceDescriptor()
          .setName("testBuffer1")
          .setFormat(FormatType::Raw32)
          .setWidth(32)
          .setDimension(FormatDimension::Buffer)
          .setUsage(ResourceUsage::GpuReadOnly);

        auto bufferdesc2 = ResourceDescriptor()
          .setName("testBuffer2")
          .setFormat(FormatType::Float32)
          .setWidth(32)
          .setDimension(FormatDimension::Buffer)
          .setUsage(ResourceUsage::GpuReadOnly);

        auto bufferdesc3 = ResourceDescriptor()
          .setName("testOutBuffer")
          .setFormat(FormatType::Float32RGBA)
          .setWidth(8)
          .setDimension(FormatDimension::Buffer)
          .setUsage(ResourceUsage::GpuRW);

        auto buffer = dev.createBuffer(bufferdesc);
        auto testSRV = dev.createBufferSRV(buffer);
        auto buffer2 = dev.createBuffer(bufferdesc2);
        auto test2SRV = dev.createBufferSRV(buffer2);
        auto buffer3 = dev.createBuffer(bufferdesc3);
        auto testOut = dev.createBufferUAV(buffer3);

        auto babyInf = ShaderInputDescriptor();
        auto triangle = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
          .setVertexShader("Triangle")
          .setPixelShader("Triangle")
          .setLayout(babyInf)
          .setPrimitiveTopology(PrimitiveTopology::Triangle)
          .setRTVFormat(0, swapchain.buffers()[0].texture().desc().desc.format)
          .setRenderTargetCount(1)
          .setDepthStencil(DepthStencilDescriptor()
            .setDepthEnable(false)));

        auto triangleRP = dev.createRenderpass();

        bool closeAnyway = false;
        bool captureMouse = false;
        bool controllerConnected = false;

        while (!window.simpleReadMessages(frame++))
        {
          // update inputs and our position
          //directInputs.pollDevices(gamepad::Fic::PollOptions::AllowDeviceEnumeration);
          // update fs
          fs.updateWatchedFiles();
          if (window.hasResized() || toggleHDR)
          {
            dev.adjustSwapchain(swapchain, scdesc);
            window.resizeHandled();
            toggleHDR = false;
          }
          auto& inputs = window.inputs();

          if (inputs.isPressedThisFrame(VK_F1, 1))
          {
            window.captureMouse(true);
            captureMouse = true;
          }
          if (inputs.isPressedThisFrame(VK_F2, 1))
          {
            window.captureMouse(false);
            captureMouse = false;
          }

          if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('1', 1))
          {
            window.toggleBorderlessFullscreen();
          }

          if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('2', 1))
          {
            reInit = true;
            if (api == GraphicsApi::DX12)
              api = GraphicsApi::Vulkan;
            else
              api = GraphicsApi::DX12;
            break;
          }

          if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('3', 1))
          {
            reInit = true;
            if (preferredVendor == VendorID::Amd)
              preferredVendor = VendorID::Nvidia;
            else
              preferredVendor = VendorID::Amd;
            break;
          }

          if (frame > 10 && (closeAnyway || inputs.isPressedThisFrame(VK_ESCAPE, 1)))
          {
            break;
          }
          if (updateLog) log.update();

          // If you acquire, you must submit it. Next, try to first present empty image.
          // On vulkan, need to at least clear the image or we will just get error about it. (... well at least when the contents are invalid in the beginning.)
          std::optional<TextureRTV> obackbuffer = dev.acquirePresentableImage(swapchain);
          if (!obackbuffer.has_value())
          {
            F_ILOG("", "No backbuffer available");
            continue;
          }
          TextureRTV backbuffer = obackbuffer.value();
          CommandGraph tasks = dev.createGraph();

          {
            auto node = tasks.createPass("Triangle");
            node.acquirePresentableImage(swapchain);
            float redcolor = 0.5f;//std::sin(time.getFTime())*.5f + .5f;

            backbuffer.clearOp(float4{ 0.f, 0.f, redcolor, 1.f });
            node.renderpass(triangleRP, backbuffer);
            {
              auto binding = node.bind(triangle);

              PixelConstants consts{};
              consts.time = 0.f;
              consts.resx = backbuffer.desc().desc.width;
              consts.resy = backbuffer.desc().desc.height;
              binding.constants(consts);

              node.draw(binding, 3);
            }
            node.endRenderpass();

			      tasks.addPass(std::move(node));
          }
          {
            auto node = tasks.createPass("preparePresent");

            node.prepareForPresent(backbuffer);
            tasks.addPass(std::move(node));
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
      }
    }
    quit = true;
  };
#if 0
  main(GraphicsApi::DX12, VendorID::Amd, true);
#else
  main(GraphicsApi::Vulkan, VendorID::Nvidia, true);
  //lbs.addTask("test1", [&](size_t) {main(GraphicsApi::Vulkan, VendorID::Nvidia, true); });
  //lbs.sleepTillKeywords({ "test1" });

#endif
  /*
  RangeMath::difference({ 0, 5, 0, 4 }, { 2, 1, 1, 2 }, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({0, 5, 1, 2}, {2, 1, 0, 5}, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({1, 2, 0, 4}, {0, 5, 1, 2}, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({2, 1, 1, 2}, {0, 5, 0, 4}, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({ 0, 2, 0, 2 }, { 1, 1, 1, 1 }, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({ 0, 1, 0, 1 }, { 1, 1, 1, 1 }, [](SubresourceRange r) {printRange(r);});
  */

  log.update();
}
