#pragma once

#include "resources.hpp"

#include <string>

namespace faze
{
  struct GpuInfo;
  class GpuDevice;

  namespace backend
  {
    namespace prototypes
    {
      class HeapImpl
      {
      public:
      };

      class DeviceImpl
      {
      public:
      };

      class SubsystemImpl
      {
      public:
        virtual std::string gfxApi() = 0;
        virtual vector<GpuInfo> availableGpus() = 0;
        virtual GpuDevice createGpuDevice(FileSystem& fs, GpuInfo gpu) = 0;
        virtual ~SubsystemImpl() {}
      };
    }
  }
}