#pragma once
#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/subsystem_data.hpp"
#include "higanbana/graphics/common/gpu_group.hpp"
#include "higanbana/graphics/common/graphicssurface.hpp"
#include <higanbana/core/platform/Window.hpp>
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
      makeState(GraphicsApi::All, appName, debugLayer, appVersion, engineName, engineVersion);
    }
    GraphicsSubsystem(GraphicsApi specificAPI, const char* appName, bool debugLayer = false, unsigned appVersion = 1, const char* engineName = "higanbana", unsigned engineVersion = 1)
    {
      makeState(specificAPI, appName, debugLayer, appVersion, engineName, engineVersion);
    }
    vector<GpuInfo> availableGpus(GraphicsApi api = GraphicsApi::All, VendorID id = VendorID::All, QueryDevicesMode mode = QueryDevicesMode::UseCached) {
      auto infos = S().availableGpus(api, id, mode);
      for (auto&& info : infos)
      {
        if (info.api == GraphicsApi::All && api != info.api)
        info.api = api;
      }
      return infos;
    }
    GpuInfo getVendorDevice(GraphicsApi api = GraphicsApi::All, VendorID id = VendorID::Unknown, QueryDevicesMode mode = QueryDevicesMode::UseCached)
    {
      auto gpus = availableGpus(GraphicsApi::All, VendorID::All, mode);
      GpuInfo info;
      for (auto&& it : gpus)
      {
        if (it.api == GraphicsApi::All || it.api == api)
        {
          if (it.vendor == id || id == VendorID::All)
          {
            info = it;
            break;
          }
        }
      }
      if (api != GraphicsApi::All && info.api == GraphicsApi::All)
        info.api = api;
      return info;
    }
    GpuGroup createDevice(FileSystem& fs, GpuInfo gpu) {
      HIGAN_ASSERT(gpu.api != GraphicsApi::All, "Creating a normal device, it's illegal to specify all api's in that case. device: \"%s\"", gpu.name.c_str());
      return S().createGroup(fs, {gpu});
    }
    GpuGroup createInteroptDevice(FileSystem& fs, GpuInfo gpu) {
      HIGAN_ASSERT(gpu.api == GraphicsApi::All, "Chosen gpu for interopt should support all api's... device: \"%s\"", gpu.name.c_str());
      return S().createGroup(fs, {gpu});
    }
    GpuGroup createGroup(FileSystem& fs, vector<GpuInfo> gpus) {
      // TODO: It's kind of bad to enforce dx12 like this even though it's the only supported api for this.
      for (auto&& gpu : gpus)
        gpu.api = GraphicsApi::DX12;
      return S().createGroup(fs, gpus);
    }
    GraphicsSurface createSurface(Window& window, GpuInfo gpu) {
      return S().createSurface(window, gpu);
    }
  };
}