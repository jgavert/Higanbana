#include "resources.hpp"
#include "gpudevice.hpp"
#include "implementation.hpp"

namespace faze
{
  namespace backend
  {
    SubsystemData::SubsystemData(GraphicsApi api, const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : impl(std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion))
      , appName(appName)
      , appVersion(appVersion)
      , engineName(engineName)
      , engineVersion(engineVersion)
    {
      switch (api)
      {
#if defined(PLATFORM_WINDOWS)
      case GraphicsApi::DX12:
        impl = std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion);
        break;
#endif
      default:
        impl = std::make_shared<VulkanSubsystem>(appName, appVersion, engineName, engineVersion);
        break;
      }
    }
    std::string SubsystemData::gfxApi() { return impl->gfxApi(); }
    vector<GpuInfo> SubsystemData::availableGpus() { return impl->availableGpus(); }
    GpuDevice SubsystemData::createDevice(FileSystem& fs, GpuInfo gpu) { return impl->createGpuDevice(fs, gpu); }
  }
}

