#pragma once
#if defined(PLATFORM_WINDOWS)
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"
//#include "faze/src/new_gfx/dx12/util/ShaderStorage.hpp"

namespace faze
{
  namespace backend
  {
    class DX12Heap : public prototypes::HeapImpl
    {
    private:
      

    public:
      DX12Heap()
      {}
    };

    class DX12Device : public prototypes::DeviceImpl
    {
    private:
      GpuInfo m_info;
    public:
      DX12Device(GpuInfo info);
    };

    class DX12Subsystem : public prototypes::SubsystemImpl
    {
      vector<GpuInfo> infos;
    public:
      DX12Subsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi() override;
      vector<GpuInfo> availableGpus() override;
      GpuDevice createGpuDevice(FileSystem& fs, GpuInfo gpu) override;
    };

  }
}
#endif