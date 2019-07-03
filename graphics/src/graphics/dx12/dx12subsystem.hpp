#pragma once
#include "graphics/dx12/dx12resources.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
#include "graphics/dx12/dx12device.hpp"

namespace faze
{
  namespace backend
  {
    class DX12Subsystem : public prototypes::SubsystemImpl
    {
      vector<GpuInfo> infos;
      ComPtr<IDXGIFactory4> pFactory;
      std::vector<ComPtr<IDXGIAdapter3>> vAdapters;
      bool m_debug;
    public:
      DX12Subsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion, bool debug = false);
      std::string gfxApi() override;
      vector<GpuInfo> availableGpus() override;
      std::shared_ptr<prototypes::DeviceImpl> createGpuDevice(FileSystem& fs, GpuInfo gpu) override;
      GraphicsSurface createSurface(Window& window) override;
    };
  }
}
#endif