#pragma once

#include "faze/src/new_gfx/common/prototypes.hpp"
#include "pipeline_descriptor.hpp"

#include "core/src/datastructures/proxy.hpp"

namespace faze
{
  class ComputePipeline
  {
  public:
    std::shared_ptr<backend::prototypes::ComputePipelineImpl> impl;
    ComputePipelineDescriptor descriptor;

    ComputePipeline(ComputePipelineDescriptor desc)
      : descriptor(desc)
    {}
  };

  class GraphicsPipeline
  {
  public:
    GraphicsPipelineDescriptor::Desc descriptor;

    using HorrorPair = std::pair<size_t, std::unique_ptr<backend::prototypes::GraphicsPipelineImpl>>;
    std::shared_ptr<vector<HorrorPair>> m_pipelines;

    GraphicsPipeline(GraphicsPipelineDescriptor desc)
      : descriptor(desc.desc)
    {}
  };
}