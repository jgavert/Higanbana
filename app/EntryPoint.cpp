// EntryPoint.cpp
#ifdef WIN64
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/system/time.hpp"
#include "core/src/tests/schedulertests.hpp"
#include "core/src/tests/bitfield_tests.hpp"
#include "core/src/math/mat_templated.hpp"
#include "app/Graphics/gfxApi.hpp"

#include <cstdio>
#include <iostream>


using namespace faze;

int EntryPoint::main()
{
  Logger log;
  auto main = [=, &log](std::string name)
  {
    //LBS lbs;
    F_LOG("Hei olen %s\n", yay::message());
    WTime t;
    ivec2 ires = { 800, 600 };
    vec2 res = { static_cast<float>(ires.x()), static_cast<float>(ires.y()) };
    Window window(m_params, name, ires.x(), ires.y());
    window.open();
    GraphicsInstance devices;
    if (!devices.createInstance("faze"))
    {
      F_LOG("Failed to create Vulkan instance, exiting\n");
      log.update();
      return;
    }
    {
      GpuDevice gpu = devices.createGpuDevice();
      GraphicsQueue queue = gpu.createGraphicsQueue();

    }
  };

  main("w1");
  return 0;
}
