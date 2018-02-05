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
#include "PlayerData.hpp"

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

      // player configs

      fighter::Player p1;
      p1.position = double2(-0.35, 0);
      fighter::Player p2;
      p2.position = double2(0.35, 0);

      constexpr double LogicMultiplier = 0.1;

      auto lastUpdated = HighPrecisionClock::now();
      while (true)
      {
        if (window.simpleReadMessages(frame++))
          quit = true;
        // timer shenigans for 60fps logic update
        auto currentTime = HighPrecisionClock::now();
        auto difference = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastUpdated).count();
        while (difference < static_cast<long long>(static_cast<double>(16667) * LogicMultiplier))
        {
          currentTime = HighPrecisionClock::now();
          difference = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastUpdated).count();
        }
        lastUpdated = currentTime;

        {
          auto input = pad.readValue();
          controllerConnected = false;
          if (input.alive)
          {
            auto extractAxises = [](int16_t input, int& output)
            {
              constexpr int16_t deadzone = 4500;
              if (std::abs(input) > deadzone)
              {
                output = (input > 0) ? 1 : -1;
              }
            };
            int2 xy;
            extractAxises(input.lstick[0].value, xy.x);
            extractAxises(input.lstick[1].value, xy.y);

            if (p1.position.y == 0.0)
            {
              // can jump! or move!

              if (std::abs(xy.x) > 0)
              {
                p1.direction = xy.x;
                p1.velocity.x = 0.02;
              }
            }
          }
          // update player position

          p1.velocity = mul(p1.velocity, LogicMultiplier);
          p1.position = add(p1.position, mul(p1.direction, p1.velocity));

          p1.velocity = double2(0.0);

          if (p1.position.y < 0.0)
            p1.position.y = 0.0;

          if (p1.position.x > 1.0 - 0.15)
            p1.position.x = 1.0 - 0.15;

          if (p1.position.x < -1.0)
            p1.position.x = -1.0;

          vector<ColoredRect> renderables;

          {
            ColoredRect r{};
            r.topleft = add(p1.position, double2(0, 0.65));
            r.rightBottom = add(p1.position, double2(0.15, 0));
            r.color = float3(0.f, 0.15f, 0.55f);

            renderables.push_back(r);
          }

          {
            ColoredRect r{};
            r.topleft = add(p2.position, double2(0, 0.65));
            r.rightBottom = add(p2.position, double2(0.15, 0));
            r.color = float3(0.f, 0.15f, 0.55f);

            renderables.push_back(r);
          }

          rend.updateBoxes(renderables);
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
        time.tick();
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