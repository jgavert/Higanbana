#include "windowMain.hpp"
#include "core/system/LBS.hpp"
#include "graphics/GraphicsCore.hpp"
#include "core/filesystem/filesystem.hpp"
#include "core/Platform/Window.hpp"
#include "core/system/logger.hpp"
#include "core/global_debug.hpp"

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


void mainWindow(ProgramParams& params)
{
  Logger log;

  LBS lbs;

  auto main = [&](GraphicsApi api, VendorID preferredVendor, bool updateLog)
  {
    bool reInit = false;
    int64_t frame = 1;
    FileSystem fs("/../../data");
    bool quit = false;

    log.update();

    bool explicitID = false;
    GpuInfo gpuinfo{};

    while (true)
    {
      GraphicsSubsystem graphics("faze");
      F_LOG("Have gpu's\n");
      auto gpus = graphics.availableGpus();
      if (!explicitID)
      {
        gpuinfo = graphics.getVendorDevice(api);
        for (auto&& it : gpus)
        {
          if (it.api == api && it.vendor == preferredVendor)
          {
            gpuinfo = it;
            break;
          }
          F_LOG("\t%s: %d. %s (memory: %zdMB, api: %s)\n", toString(it.api), it.id, it.name.c_str(), it.memory/1024/1024, it.apiVersionStr.c_str());
        }
      }
      if (updateLog) log.update();
      if (gpus.empty())
        return;

	    std::string windowTitle = std::string(toString(gpuinfo.api)) + ": " + gpuinfo.name;
      Window window(params, windowTitle, 1280, 720, 300, 200);
      window.open();

      auto surface = graphics.createSurface(window, gpuinfo);
      auto dev = graphics.createGroup(fs, {gpuinfo});
      {
        auto toggleHDR = false;
        auto scdesc = SwapchainDescriptor()
          .formatType(FormatType::Unorm8RGBA)
          .colorspace(Colorspace::BT709)
          .bufferCount(3).presentMode(PresentMode::Fifo);
        auto swapchain = dev.createSwapchain(surface, scdesc);

        F_LOG("Created device \"%s\"\n", gpuinfo.name.c_str());

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
          //dev.present(swapchain);
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
