#pragma once
#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/subsystem_data.hpp"
#include "higanbana/graphics/common/gpu_group.hpp"
#include "higanbana/graphics/common/graphicssurface.hpp"
#include <higanbana/core/Platform/Window.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <string>

namespace higanbana
{
  class GraphicsSubsystem : private backend::SharedState<backend::SubsystemData>
  {
  public:

    GraphicsSubsystem() = default;
    GraphicsSubsystem(const char* appName, bool debugLayer = false, unsigned appVersion = 1, const char* engineName = "higanbana", unsigned engineVersion = 1)
    {
      makeState(appName, debugLayer, appVersion, engineName, engineVersion);
    }
    vector<GpuInfo> availableGpus(GraphicsApi api = GraphicsApi::All, VendorID id = VendorID::All) { return S().availableGpus(api, id); }
    GpuInfo getVendorDevice(GraphicsApi api, VendorID id = VendorID::Unknown)
    {
      auto gpus = availableGpus();
      GpuInfo info = gpus[0];
      for (auto&& it : gpus)
      {
        if (it.api != api)
          continue;
        if (info.api != api && it.api == api)
          info = it;
        if (it.vendor == id || id == VendorID::Unknown)
        {
          info = it;
          break;
        }
      }
      return info;
    }
    GpuGroup createGroup(FileSystem& fs, vector<GpuInfo> gpu) { return S().createGroup(fs, gpu); }
    GraphicsSurface createSurface(Window& window, GpuInfo gpu) { return S().createSurface(window, gpu); }
  };
}