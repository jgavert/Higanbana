#pragma once

#include "pipeline_descriptor.hpp"

namespace faze
{
  class ComputePipeline
  {
  public:
    ComputePipelineDescriptor descriptor;

    ComputePipeline(ComputePipelineDescriptor desc)
      : descriptor(desc)
    {}
  };

  class GraphicsPipeline
  {
  public:
    GraphicsPipelineDescriptor::Desc descriptor;

    GraphicsPipeline(GraphicsPipelineDescriptor desc)
      : descriptor(desc.desc)
    {}
  };
}