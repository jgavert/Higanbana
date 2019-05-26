#pragma once

#include "graphics/common/prototypes.hpp"
#include "graphics/common/pipeline_descriptor.hpp"

#include "core/datastructures/proxy.hpp"

#include "core/filesystem/filesystem.hpp"

namespace faze
{
  class ComputePipeline
  {
  public:
    std::shared_ptr<backend::prototypes::PipelineImpl> impl;
    std::shared_ptr<WatchFile> m_update;
    ComputePipelineDescriptor descriptor;

    ComputePipeline()
    {}

    ComputePipeline(std::shared_ptr<backend::prototypes::PipelineImpl> impl, ComputePipelineDescriptor desc)
      : impl(impl)
      , m_update(std::make_shared<WatchFile>())
      , descriptor(desc)
    {}
  };

  class GraphicsPipeline
  {
  public:
    GraphicsPipelineDescriptor descriptor;

    struct FullPipeline
    {
      size_t hash;
      std::shared_ptr<backend::prototypes::PipelineImpl> pipeline;
      WatchFile vs;
      WatchFile ds;
      WatchFile hs;
      WatchFile gs;
      WatchFile ps;

      bool needsUpdating()
      {
        return !pipeline || vs.updated() || ps.updated() || ds.updated() || hs.updated() || gs.updated();
      }

      void updated()
      {
        vs.react();
        ds.react();
        hs.react();
        gs.react();
        ps.react();
      }
    };

    std::shared_ptr<vector<FullPipeline>> m_pipelines;

    GraphicsPipeline()
      : descriptor()
      , m_pipelines(nullptr)
    {}

    GraphicsPipeline(GraphicsPipelineDescriptor desc)
      : descriptor(desc)
      , m_pipelines(std::make_shared<vector<FullPipeline>>())
    {}
  };
}