#if defined(PLATFORM_WINDOWS)
#include "dx12resources.hpp"
#include "faze/src/new_gfx/common/gpudevice.hpp"
#include "core/src/global_debug.hpp"


namespace faze
{
  namespace backend
  {
    DX12Subsystem::DX12Subsystem(const char* , unsigned , const char* , unsigned )
    {
  
    }

    std::string DX12Subsystem::gfxApi()
    {
      return "DX12";
    }

    vector<GpuInfo> DX12Subsystem::availableGpus()
    {
      return infos;
    }

    GpuDevice DX12Subsystem::createGpuDevice(FileSystem& , GpuInfo gpu)
    {
       return GpuDevice(DeviceData(std::make_shared<DX12Device>(gpu)));
    }
  }
}
#endif