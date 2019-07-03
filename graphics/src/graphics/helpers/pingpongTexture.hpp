#pragma once

#include "graphics/GraphicsCore.hpp"

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

    PingPongTexture(GpuGroup gpu, const ResourceDescriptor& desc)
    {
      resize(gpu, desc);
    }

    void resize(GpuGroup gpu, const ResourceDescriptor& desc)
    {
      srvs.clear();
      uavs.clear();
      rtvs.clear();
      for (int i = 0; i < maxTextures; ++i)
      {
        auto res = gpu.createTexture(desc);
        srvs.push_back(gpu.createTextureSRV(res));
        if (desc.desc.usage == ResourceUsage::DepthStencilRW || desc.desc.usage == ResourceUsage::RenderTargetRW || desc.desc.usage == ResourceUsage::GpuRW)
          uavs.push_back(gpu.createTextureUAV(res));
        if (desc.desc.usage == ResourceUsage::RenderTarget || desc.desc.usage == ResourceUsage::RenderTargetRW)
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

    Texture& texture()
    {
      return srvs[currentIndex].texture();
    }
  };
}