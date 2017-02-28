#pragma once
#include "backend.hpp"
#include "resources.hpp"
#include "graphicssurface.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/datastructures/proxy.hpp"
#include <string>

namespace faze
{
  class GraphicsSubsystem : private backend::SharedState<backend::SubsystemData>
  {
  public:

    GraphicsSubsystem() = default;
    GraphicsSubsystem(GraphicsApi api, const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1)
    {
      makeState(api, appName, appVersion, engineName, engineVersion);
    }
    std::string gfxApi() { return S().gfxApi(); }
    vector<GpuInfo> availableGpus() { return S().availableGpus(); }
    GpuDevice createDevice(FileSystem& fs, GpuInfo gpu) { return GpuDevice(S().createDevice(fs, gpu)); }
    GraphicsSurface createSurface(Window& window) { return S().createSurface(window); }
  };
}