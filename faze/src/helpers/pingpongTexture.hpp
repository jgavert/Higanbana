#pragma once

#include "faze/src/new_gfx/GraphicsCore.hpp"

namespace faze
{
  class PingPongTexture
  {
    vector<TextureSRV> srvs;
    vector<TextureUAV> uavs;
    vector<TextureRTV> rtvs;
    int currentIndex = 0;
    int maxTextures = 2;
  public:

    PingPongTexture()
    {
    }

    PingPongTexture(GpuDevice gpu, const ResourceDescriptor& desc)
    {
      resize(gpu, desc);
    }

    void resize(GpuDevice gpu, const ResourceDescriptor& desc)
    {
      srvs.clear();
      uavs.clear();
      rtvs.clear();
      for (int i = 0; i < maxTextures; ++i)
      {
        auto res = gpu.createTexture(desc);
        srvs.push_back(gpu.createTextureSRV(res));
        uavs.push_back(gpu.createTextureUAV(res));
        rtvs.push_back(gpu.createTextureRTV(res));
      }
    }

    ResourceDescriptor desc()
    {
      return srvs[0].desc();
    }

    void next()
    {
      currentIndex = (currentIndex + 1) % maxTextures;
    }

    TextureSRV& srv()
    {
      return srvs[currentIndex];
    }

    TextureUAV& uav()
    {
      return uavs[currentIndex];
    }

    TextureRTV& rtv()
    {
      return rtvs[currentIndex];
    }
  };
}