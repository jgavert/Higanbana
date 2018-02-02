#include "fighterMain.hpp"

#if defined(FAZE_PLATFORM_WINDOWS)

#include "core/src/neural/network.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/time.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/global_debug.hpp"

#include "core/src/math/math.hpp"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "core/src/external/imgiu/imgui.h"

#include "core/src/input/gamepad.hpp"
#include "core/src/system/AtomicBuffered.hpp"

#include "faze/src/new_gfx/GraphicsCore.hpp"

#include "fighterRenderer.hpp"

#include <tuple>

using namespace faze;
using namespace faze::math;

void fighterWindow(ProgramParams& params)
{
  Logger log;
  LBS lbs;

  std::atomic<bool> quitLogging = false;
  lbs.addTask("background log log", [&](size_t)
  {
    if (quitLogging)
    {
      lbs.addTask("logging finished", [&](size_t)
      {
      });
      return;
    }
    log.update();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    rescheduleTask();
  });

  auto main = [&](GraphicsApi api, VendorID preferredVendor)
  {
    int64_t frame = 1;
    FileSystem fs;
    WTime time;
    WTime controllerTime;

    gamepad::Fic directInputs;
    AtomicDoubleBuffered<gamepad::X360LikePad> pad;
    std::atomic<bool> quit = false;
    controllerTime.firstTick();
    lbs.addTask("gamepad update", [&](size_t)
    {
      controllerTime.tick();
      if (quit)
      {
        lbs.addTask("gamepadDone", [&](size_t)
        {
        });
        return;
      }
      directInputs.pollDevices(gamepad::Fic::PollOptions::AllowDeviceEnumeration);
      pad.writeValue(directInputs.getFirstActiveGamepad());
      rescheduleTask();
    });

    GraphicsSubsystem graphics(api, "faze");
    F_LOG("Using api %s\n", graphics.gfxApi().c_str());
    F_ASSERT(!graphics.availableGpus().empty(), "No valid gpu's available.\n");

    auto gpuInfo = graphics.getVendorDevice(preferredVendor);

    Window window(params, gpuInfo.name, 1280, 720, 300, 200);
    window.open();

    auto surface = graphics.createSurface(window);
    auto dev = graphics.createDevice(fs, gpuInfo);

    FighterRenderer rend(surface, dev);

    lbs.addTask("game rendering", [&](size_t)
    {
      if (quit)
      {
        lbs.addTask("renderingDone", [&](size_t)
        {
        });
        return;
      }
      rend.render();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      rescheduleTask();
    });

    time.firstTick();
    {
      bool controllerConnected = false;

      while (!window.simpleReadMessages(frame++))
      {
        {
          auto input = pad.readValue();
          controllerConnected = false;
          if (input.alive)
          {
          }
        }

        // update fs
        fs.updateWatchedFiles();

        if (window.hasResized())
        {
          rend.resize();
          window.resizeHandled();
        }
        auto& inputs = window.inputs();

        if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('1', 1))
        {
          window.toggleBorderlessFullscreen();
        }

        if (inputs.isPressedThisFrame(VK_ESCAPE, 1))
        {
          quit = true;
        }

        // If you acquire, you must submit it. Next, try to first present empty image.
        // On vulkan, need to at least clear the image or we will just get error about it. (... well at least when the contents are invalid in the beginning.)

        //end of mainloop
        if (quit)
        {
          lbs.sleepTillKeywords({ "gamepadDone", "renderingDone" });
          break;
        }
      }
    }
  };

  // start the app
  main(GraphicsApi::DX12, VendorID::Amd);

  quitLogging = true;
  lbs.sleepTillKeywords({ "logging finished" });
  log.update();
}

#else

void mainWindow(ProgramParams&)
{
}

#endif