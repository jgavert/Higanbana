#pragma once

#include "faze/src/new_gfx/common/prototypes.hpp"
#include "pipeline_descriptor.hpp"

#include "core/src/datastructures/proxy.hpp"

namespace faze
{
  class ComputePipeline
  {
  public:
    std::shared_ptr<backend::prototypes::PipelineImpl> impl;
    ComputePipelineDescriptor descriptor;

    ComputePipeline(std::shared_ptr<backend::prototypes::PipelineImpl> impl, ComputePipelineDescriptor desc)
      : impl(impl)
      , descriptor(desc)
    {}
  };

  class GraphicsPipeline
  {
  public:
    GraphicsPipelineDescriptor::Desc descriptor;

    using HorrorPair = std::pair<size_t, std::shared_ptr<backend::prototypes::PipelineImpl>>;
    std::shared_ptr<vector<HorrorPair>> m_pipelines;

    GraphicsPipeline(GraphicsPipelineDescriptor desc)
      : descriptor(desc.desc)
      , m_pipelines(std::make_shared<vector<HorrorPair>>())
    {}
  };
}