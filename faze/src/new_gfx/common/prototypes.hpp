#pragma once

#include "resources.hpp"
#include "heap_descriptor.hpp"
#include <string>

namespace faze
{
  struct GpuInfo;
  class GpuDevice;

  namespace backend
  {
    struct GpuHeap;

    namespace prototypes
    {
      class TextureImpl
      {
      public:
      };
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

        virtual std::shared_ptr<BufferImpl> createBuffer(HeapAllocation allocation, ResourceDescriptor desc) = 0;
        virtual void destroyBuffer(std::shared_ptr<BufferImpl> buffer) = 0;
        virtual void createBufferView(ShaderViewDescriptor desc) = 0;

        virtual std::shared_ptr<TextureImpl> createTexture(HeapAllocation allocation, ResourceDescriptor desc) = 0;
        virtual void destroyTexture(std::shared_ptr<TextureImpl> buffer) = 0;
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