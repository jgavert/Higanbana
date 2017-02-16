#pragma once

#include "resources.hpp"
#include "heap_descriptor.hpp"
#include <string>

namespace faze
{
  struct GpuInfo;
  class GpuDevice;
  class Buffer;

  namespace backend
  {
    struct GpuHeap;

    namespace prototypes
    {
      class BufferImpl
      {
      public:
      };

      class HeapImpl
      {
      public:
      };

      class DeviceImpl
      {
      public:

        // utility
        virtual void waitGpuIdle() = 0;
        virtual MemoryRequirements getReqs(ResourceDescriptor desc) = 0;

        //create/destroy pairs 
        virtual GpuHeap createHeap(HeapDescriptor desc) = 0;
        virtual void destroyHeap(GpuHeap heap) = 0;

        virtual backend::BufferData createBuffer(HeapAllocation allocation, ResourceDescriptor desc) = 0;
        virtual void destroyBuffer(Buffer buffer) = 0;
        virtual void createBufferView(ShaderViewDescriptor desc) = 0;
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