#pragma once

#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/common/pipeline_descriptor.hpp"

#include <higanbana/core/datastructures/vector.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>

namespace higanbana
{
  class ComputePipeline
  {
  public:
    std::shared_ptr<ResourceHandle> impl;
    std::shared_ptr<WatchFile> m_update;
    ComputePipelineDescriptor descriptor;

    ComputePipeline()
    {}

    ComputePipeline(std::shared_ptr<ResourceHandle> impl, ComputePipelineDescriptor desc)
      : impl(impl)
      , m_update(std::make_shared<WatchFile>())
      , descriptor(desc)
    {}
    ResourceHandle handle() const
    {
      if (impl)
        return *impl;
      return ResourceHandle();
    }
    operator bool() const {
      return handle().id != ResourceHandle::InvalidId;
    }
  };

  class GraphicsPipeline
  {
  public:
    std::shared_ptr<ResourceHandle> pipeline;
    GraphicsPipelineDescriptor descriptor;

    WatchFile vs;
    WatchFile ds;
    WatchFile hs;
    WatchFile gs;
    WatchFile ps;

    bool needsUpdating()
    {
      return vs.updated() || ps.updated() || ds.updated() || hs.updated() || gs.updated();
    }

    void updated()
    {
      vs.react();
      ds.react();
      hs.react();
      gs.react();
      ps.react();
    }

    GraphicsPipeline()
      : descriptor()
    {}

    GraphicsPipeline(std::shared_ptr<ResourceHandle> handle, GraphicsPipelineDescriptor desc)
      : pipeline(handle)
      , descriptor(desc)
    {}
    ResourceHandle handle() const
    {
      if (pipeline)
        return *pipeline;
      return ResourceHandle();
    }
    operator bool() const {
      return handle().id != ResourceHandle::InvalidId;
    }
  };
}