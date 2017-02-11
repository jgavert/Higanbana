#pragma once
#include "backend.hpp"
#include "resources.hpp"

#include "core/src/datastructures/proxy.hpp"
#include <string>

namespace faze
{
  class GraphicsSubsystem : private backend::SharedState<backend::SubsystemData>
  {
  public:

    GraphicsSubsystem() = default;
    GraphicsSubsystem(const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1)
    {
      makeState(appName, appVersion, engineName, engineVersion);
    }
    std::string gfxApi() { return S().gfxApi(); }
    vector<GpuInfo> availableGpus() { return S().availableGpus(); }
    GpuDevice createDevice(FileSystem& fs, int id) { return GpuDevice(S().createDevice(fs, id)); }
  };
}