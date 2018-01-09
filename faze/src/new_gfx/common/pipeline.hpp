#pragma once

#include "faze/src/new_gfx/common/prototypes.hpp"
#include "pipeline_descriptor.hpp"

#include "core/src/datastructures/proxy.hpp"

#include "core/src/filesystem/filesystem.hpp"

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
    GraphicsPipelineDescriptor::Desc descriptor;

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
      : descriptor(desc.desc)
      , m_pipelines(std::make_shared<vector<FullPipeline>>())
    {}
  };
}