#include "mipmapper.hpp"

#include "shaders/mipmapper.if.hpp"

namespace faze
{
  namespace renderer
  {
    Mipmapper::Mipmapper(GpuDevice& device)
    {
      pipeline = device.createComputePipeline<::shader::Mipmapper>(ComputePipelineDescriptor()
        .setShader("mipmapper")
        .setThreadGroups(uint3(8, 8, 1)));
    }

    void Mipmapper::generateMip(CommandGraphNode& graph, TextureUAV& target, TextureSRV& source, float2 texelSize, int2 dim)
    {
      auto binding = graph.bind<::shader::Mipmapper>(pipeline);
      binding.constants.texelSize = texelSize;
      binding.constants.mipLevelsToGenerate = 1;

      binding.srv(::shader::Mipmapper::sourceMip0, source);
      binding.uav(::shader::Mipmapper::mip1, target);
      binding.uav(::shader::Mipmapper::mip2, target);
      binding.uav(::shader::Mipmapper::mip3, target);
      binding.uav(::shader::Mipmapper::mip4, target);

      unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(dim.x), 8));
      unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(dim.y), 8));
      uint3 groups = uint3(x, y, 1);
      graph.dispatch(binding, groups);
    }

    void Mipmapper::generateMipLevels(GpuDevice& device, CommandGraphNode& graph, Texture& source, int startMip)
    {
      // calculate how many mipmaps can be generated from sourceSRV
      int3 dim = int3(source.size3D());
      vector<int3> mipchain;
      while (!(dim.x == 1 && dim.y == 1 && dim.z == 1))
      {
        mipchain.push_back(dim);
        dim = calculateMipDim(dim, 1);
      }
      mipchain.push_back(dim); // add last mip

      int totalMipLevels = source.desc().desc.miplevels; // cannot generate more than we have room
      int missingMips = totalMipLevels - startMip;
      // create textureUAV's for each mip

      for (int i = 0; i < missingMips-1; ++i)
      {
        auto mipDim = mipchain[startMip + i+1];
        {
          auto texelSize = float2(1.f / mipDim.x, 1.f / mipDim.y);
          auto srv = device.createTextureSRV(source, ShaderViewDescriptor().setMostDetailedMip(startMip + i).setMipLevels(1));
          auto uav = device.createTextureUAV(source, ShaderViewDescriptor().setMostDetailedMip(startMip + i + 1).setMipLevels(1));
          generateMip(graph, uav, srv, texelSize, mipDim.xy());
        }
      }
      // call in loop until all mip's area created
    }

    void Mipmapper::generateMipLevelsTo(GpuDevice& device, CommandGraphNode& graph, Texture& target, Texture& source, int startMip)
    {
      // calculate how many mipmaps can be generated from sourceSRV
      int3 dim = int3(source.size3D());
      vector<int3> mipchain;
      while (!(dim.x == 1 && dim.y == 1 && dim.z == 1))
      {
        mipchain.push_back(dim);
        dim = calculateMipDim(dim, 1);
      }
      mipchain.push_back(dim); // add last mip

      int totalMipLevels = target.desc().desc.miplevels; // cannot generate more than we have room
      int missingMips = totalMipLevels - startMip;
      // create textureUAV's for each mip

      auto texelSize = float2(1.f / mipchain[startMip+1].x, 1.f / mipchain[startMip + 1].y);
      auto srv = device.createTextureSRV(source, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
      auto uav = device.createTextureUAV(target, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
      generateMip(graph, uav, srv, texelSize, mipchain[startMip + 1].xy());

      for (int i = 0; i < missingMips - 1; ++i)
      {
        auto mipDim = mipchain[startMip + i + 2];
        {
          texelSize = float2(1.f / mipDim.x, 1.f / mipDim.y);
          srv = device.createTextureSRV(target, ShaderViewDescriptor().setMostDetailedMip(startMip + i).setMipLevels(1));
          uav = device.createTextureUAV(target, ShaderViewDescriptor().setMostDetailedMip(startMip + i + 1).setMipLevels(1));
          generateMip(graph, uav, srv, texelSize, mipDim.xy());
        }
      }
      // call in loop until all mip's area created
    }
  }
}