#pragma once

#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "gfxDebug.hpp"
#include <utility>
#include <string>


struct GpuInfo
{
  std::string description;
  unsigned vendorId;
  unsigned deviceId;
  unsigned subSysId;
  unsigned revision;
  size_t dedVidMem;
  size_t dedSysMem;
  size_t shaSysMem;
};

class SystemDevices
{
private:
  std::vector<GpuInfo> infos;
  // 2 device support only
  int betterDevice;
  int lesserDevice;

public:
  SystemDevices() : betterDevice(-1), lesserDevice(-1)
  {
  }

  GpuDevice CreateGpuDevice(bool debug = true, bool warpDriver = true)
  {
#ifdef DEBUG
    debug = true;
	  warpDriver = false;
#endif
    return CreateGpuDevice(betterDevice, debug, warpDriver);
  }

  GpuDevice CreateGpuDevice(int , bool debug /*= false*/, bool  /*warpDevice = true*/)
  {
    return GpuDevice(nullptr, debug);
  }

  GpuInfo getInfo(int num)
  {
    if (num < 0 || num >= infos.size())
      return{};
    return infos[num];
  }

  int DeviceCount()
  {
    return static_cast<int>(infos.size());
  }

};
