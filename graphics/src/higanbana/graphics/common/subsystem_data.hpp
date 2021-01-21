#pragma once
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resources/graphics_api.hpp"
#include "higanbana/core/datastructures/proxy.hpp"

namespace higanbana
{
  class GpuGroup;
  class FileSystem;
  class Window;

  namespace backend
  {
    struct SubsystemData : std::enable_shared_from_this<SubsystemData>
    {
      std::shared_ptr<prototypes::SubsystemImpl> implDX12;
      std::shared_ptr<prototypes::SubsystemImpl> implVulkan;
      const char* appName;
      unsigned appVersion;
      const char* engineName;
      unsigned engineVersion;

      vector<GpuInfo> m_cachedInfos;

      SubsystemData(GraphicsApi allowedApi, const char* appName, bool debugLayer, unsigned appVersion = 1, const char* engineName = "higanbana", unsigned engineVersion = 1);
      vector<GpuInfo> availableGpus(GraphicsApi api = GraphicsApi::All, VendorID id = VendorID::All, QueryDevicesMode mode = QueryDevicesMode::UseCached);
      GpuGroup createGroup(FileSystem& fs, vector<GpuInfo> gpu);
      GraphicsSurface createSurface(Window& window, GpuInfo gpu);
    };
  }
}