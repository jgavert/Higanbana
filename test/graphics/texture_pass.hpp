#pragma once

#include "faze/src/new_gfx/GraphicsCore.hpp"
#include "core/src/math/math.hpp"
#include <string>
#include <algorithm>

namespace faze
{
  namespace renderer
  {
    template <typename Shader>
    class TexturePass
    {
      ComputePipeline pipeline;
      std::string shaderName;
      uint3 threadGroups;
    public:
      TexturePass(GpuDevice& device, std::string shaderName, uint3 threadGroupCounts)
        : shaderName(shaderName)
        , threadGroups(threadGroupCounts)
      {
        faze::ComputePipelineDescriptor dsc = faze::ComputePipelineDescriptor().setShader(shaderName).setThreadGroups(threadGroupCounts);
        pipeline = device.createComputePipeline<Shader>(dsc);
      }

      void compute(CommandGraphNode& node, TextureUAV& target, TextureSRV& source, uint2 shaderThreads)
      {
        auto binding = node.bind<Shader>(pipeline);

        binding.constants = {};
        binding.constants.sourceRes = uint2{ source.desc().desc.width, source.desc().desc.height };
        binding.constants.targetRes = uint2{ target.desc().desc.width, target.desc().desc.height };

        binding.srv(Shader::source, source);
        binding.uav(Shader::target, target);

        uint2 res = shaderThreads;

        res.x = std::max(res.x, 1u);
        res.y = std::max(res.y, 1u);

        uint2 txy{ 1,1 };
        if (threadGroups.y > 1)
        {
          txy.x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(res.x), static_cast<uint64_t>(threadGroups.x)));
          txy.y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(res.y), static_cast<uint64_t>(threadGroups.y)));
        }
        else
        {
          txy.x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(res.x*res.y), static_cast<uint64_t>(threadGroups.x)));
        }

        node.dispatch(binding, uint3(txy, 1));
      }
    };
  }
}