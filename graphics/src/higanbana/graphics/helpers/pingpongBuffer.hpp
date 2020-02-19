#pragma once

#include "higanbana/graphics/GraphicsCore.hpp"

namespace higanbana
{
  class PingPongBuffer
  {
    vector<BufferSRV> srvs;
    vector<BufferUAV> uavs;
    int currentIndex = 0;
    int maxBuffers = 2;
  public:

    PingPongBuffer()
    {
    }

    PingPongBuffer(GpuGroup gpu, const ResourceDescriptor& desc)
    {
      resize(gpu, desc);
    }

    void resize(GpuGroup gpu, const ResourceDescriptor& desc)
    {
      srvs.clear();
      uavs.clear();
      for (int i = 0; i < maxBuffers; ++i)
      {
        auto res = gpu.createBuffer(desc);
        srvs.push_back(gpu.createBufferSRV(res));
        if (desc.desc.usage == ResourceUsage::DepthStencilRW || desc.desc.usage == ResourceUsage::RenderTargetRW || desc.desc.usage == ResourceUsage::GpuRW)
          uavs.push_back(gpu.createBufferUAV(res));
      }
    }

    ResourceDescriptor desc()
    {
      return srvs[0].desc();
    }

    void next()
    {
      currentIndex = (currentIndex + 1) % maxBuffers;
    }
    BufferSRV& currentSrv()
    {
      return srvs[currentIndex];
    }

    BufferUAV& currentUav()
    {
      return uavs[currentIndex];
    }

    Buffer& currentBuffer()
    {
      return srvs[currentIndex].buffer();
    }

    BufferSRV& previousSrv()
    {
      auto index = (currentIndex - 1);
      index = index < 0 ? maxBuffers-1 : index;
      return srvs[index];
    }

    BufferUAV& previousUav()
    {
      auto index = (currentIndex - 1);
      index = index < 0 ? maxBuffers-1 : index;
      return uavs[index];
    }

    Buffer& previousBuffer()
    {
      auto index = (currentIndex - 1);
      index = index < 0 ? maxBuffers-1 : index;
      return srvs[index].buffer();
    }
  };
}