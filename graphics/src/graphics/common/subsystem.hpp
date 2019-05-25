#pragma once
#include "graphics/common/backend.hpp"
#include "graphics/common/subsystem_data.hpp"
#include "graphics/common/graphicssurface.hpp"
#include "core/Platform/Window.hpp"
#include "core/datastructures/proxy.hpp"
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
    vector<GpuInfo> availableGpus(GraphicsApi api = GraphicsApi::All) { return S().availableGpus(api); }
    GpuInfo getVendorDevice(GraphicsApi api, VendorID id = VendorID::Unknown)
    {
      auto gpus = availableGpus();
      GpuInfo info = gpus[0];
      for (auto&& it : gpus)
      {
        if (it.api != api)
          continue;
        if (it.vendor == id || id == VendorID::Unknown)
        {
          info = it;
          break;
        }
      }
      return info;
    }
    GpuDevice createDevice(FileSystem& fs, GpuInfo gpu) { return GpuDevice(S().createDevice(fs, gpu)); }
    GraphicsSurface createSurface(Window& window, GpuInfo gpu) { return S().createSurface(window, gpu); }
  };
}