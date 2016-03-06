#pragma once

#include <string>

class GpuInfo
{
public:
  GpuInfo()
    : vendorId(0)
    , deviceId(0)
    , subSysId(0)
    , revision(0)
    , dedVidMem(0)
    , shaSysMem(0)
  {}
  std::string description;
  unsigned vendorId;
  unsigned deviceId;
  unsigned subSysId;
  unsigned revision;
  size_t dedVidMem;
  size_t dedSysMem;
  size_t shaSysMem;
};