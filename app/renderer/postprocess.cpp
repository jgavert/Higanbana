#include "postprocess.hpp"

#include "shaders/postprocess.if.hpp"

namespace faze
{
  namespace renderer
  {
    Postprocess::Postprocess(GpuDevice& device)
    {
      pipeline = device.createComputePipeline<::shader::Postprocess>(ComputePipelineDescriptor()
        .setShader("postprocess")
        .setThreadGroups(uint3(8, 8, 1)));
    }

    void Postprocess::postprocess(CommandGraphNode& graph, TextureUAV& target, TextureSRV& source, TextureSRV& luminance)
    {
      auto binding = graph.bind<::shader::Postprocess>(pipeline);
      binding.srv(::shader::Postprocess::source, source);
      binding.srv(::shader::Postprocess::luminance, luminance);
      binding.uav(::shader::Postprocess::output, target);

      auto dim = source.texture().size3D();
      unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(dim.x), 8));
      unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(dim.y), 8));
      uint3 groups = uint3(x, y, 1);
      graph.dispatch(binding, groups);
    }
  }
}