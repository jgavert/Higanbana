#include "resources.hpp"
#include "gpudevice.hpp"
#include "implementation.hpp"

namespace faze
{
  namespace backend
  {
    SubsystemData::SubsystemData(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : impl(std::make_shared<SubsystemImpl>(appName, appVersion, engineName, engineVersion))
      , appName(appName)
      , appVersion(appVersion)
      , engineName(engineName)
      , engineVersion(engineVersion)
    {
    }
    std::string SubsystemData::gfxApi() { return impl->gfxApi(); }
    vector<GpuInfo> SubsystemData::availableGpus() { return impl->availableGpus(); }
    GpuDevice SubsystemData::createDevice(FileSystem& fs, int id) { return impl->createGpuDevice(fs, id); }
  }
}

