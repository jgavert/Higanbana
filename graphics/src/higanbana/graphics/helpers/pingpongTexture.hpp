#pragma once

#include "higanbana/graphics/GraphicsCore.hpp"

namespace higanbana
{
  class PingPongTexture
  {
    vector<TextureSRV> srvs;
    vector<TextureUAV> uavs;
    vector<TextureRTV> rtvs;
    int currentIndex = 0;
    int maxTextures = 2;

    int previousIndex()
    {
      auto index = (currentIndex - 1);
      index = index < 0 ? maxTextures-1 : index;
      return index;
    }
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
      HIGAN_ASSERT(!srvs.empty(), "No texture exists!, please resize.");
      return srvs[0].texture().desc();
    }

    void next()
    {
      currentIndex = (currentIndex + 1) % maxTextures;
    }

    TextureSRV& srv()
    {
      HIGAN_ASSERT(srvs.size() > currentIndex, "Invalid index %d < size:%zu", currentIndex, srvs.size());
      return srvs[currentIndex];
    }

    TextureUAV& uav()
    {
      HIGAN_ASSERT(uavs.size() > currentIndex, "Invalid index %d < size:%zu", currentIndex, uavs.size());
      return uavs[currentIndex];
    }

    TextureRTV& rtv()
    {
      HIGAN_ASSERT(rtvs.size() > currentIndex, "Invalid index %d < size:%zu", currentIndex, rtvs.size());
      return rtvs[currentIndex];
    }

    Texture& texture()
    {
      HIGAN_ASSERT(srvs.size() > currentIndex, "Invalid index %d < size:%zu", currentIndex, srvs.size());
      return srvs[currentIndex].texture();
    }


    TextureSRV& previousSrv()
    {
      auto index = previousIndex();
      HIGAN_ASSERT(srvs.size() > index, "Invalid index %d < size:%zu", index, srvs.size());
      return srvs[index];
    }

    TextureUAV& previousUav()
    {
      auto index = previousIndex();
      HIGAN_ASSERT(uavs.size() > index, "Invalid index %d < size:%zu", index, uavs.size());
      return uavs[index];
    }

    TextureRTV& previousRtv()
    {
      auto index = previousIndex();
      HIGAN_ASSERT(rtvs.size() > index, "Invalid index %d < size:%zu", index, rtvs.size());
      return rtvs[currentIndex];
    }

    Texture& previousTexture()
    {
      auto index = previousIndex();
      HIGAN_ASSERT(srvs.size() > index, "Invalid index %d < size:%zu", index, srvs.size());
      return srvs[index].texture();
    }
  };
}