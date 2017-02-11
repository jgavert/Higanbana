#pragma once

#include "backend.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/datastructures/proxy.hpp"
#include <string>
namespace faze
{
  enum class DeviceType
  {
    Unknown,
    IntegratedGpu,
    DiscreteGpu,
    VirtualGpu,
    Cpu
  };

  struct GpuInfo
  {
    int id;
    std::string name;
    int64_t memory;
    int vendor;
    DeviceType type;
    bool canPresent;
  };

  namespace backend
  {
    class DeviceImpl;
    class SubsystemImpl;

    struct DeviceData : std::enable_shared_from_this<DeviceData>
    {
      std::shared_ptr<DeviceImpl> impl;

      DeviceData(std::shared_ptr<DeviceImpl> impl)
        : impl(impl)
      {
      }
    };

    struct SubsystemData : std::enable_shared_from_this<SubsystemData>
    {
      std::shared_ptr<SubsystemImpl> impl; // who creates this? default constructable?
      const char* appName;
      unsigned appVersion;
      const char* engineName;
      unsigned engineVersion;

      SubsystemData(const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
      GpuDevice createDevice(FileSystem& fs, int id);
    };
  }
}