#include "windowMain.hpp"
#include "core/system/LBS.hpp"
#include "graphics/GraphicsCore.hpp"
#include "core/filesystem/filesystem.hpp"
#include "core/Platform/Window.hpp"
#include "core/system/logger.hpp"
#include "core/system/time.hpp"
#include "core/global_debug.hpp"

#include "entity_test.hpp"
#include <tuple>
#include <core/system/misc.hpp>
#include <random>

#include "rendering.hpp"

using namespace faze;
using namespace faze::math;

void mainWindow(ProgramParams& params)
{
  Logger log;
  // test_entity();
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
      if (allGpus.empty())
      {
        for (auto&& it : gpus)
        {
          if (it.api == api)
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

      auto dev = graphics.createGroup(fs, allGpus);
      app::Renderer rend(graphics, dev);
      {
        auto toggleHDR = false;
        rend.initWindow(window, allGpus[0]);

        for (auto& gpu : allGpus)
        {
          F_LOG("Created device \"%s\"\n", gpu.name.c_str());
        }

        bool closeAnyway = false;
        bool captureMouse = false;
        bool controllerConnected = false;

        while (!window.simpleReadMessages(frame++))
        {
          // update inputs and our position
          //directInputs.pollDevices(gamepad::Fic::PollOptions::AllowDeviceEnumeration);
          // update fs
          log.update();
          fs.updateWatchedFiles();
          if (window.hasResized() || toggleHDR)
          {
            //dev.adjustSwapchain(swapchain, scdesc);
            rend.windowResized();
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

          rend.render();
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
