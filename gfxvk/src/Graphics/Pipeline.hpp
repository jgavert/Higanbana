#pragma once

#include "vk_specifics/VulkanPipeline.hpp"

class GpuPipeline
{
	friend class GpuDevice;
  friend class GraphicsCmdBuffer;
  friend class GraphicsPipeline;
  friend class ComputePipeline;

  PipelineImpl m_pipeline;

  GpuPipeline(PipelineImpl pipeline) : m_pipeline(pipeline) {}
public:
  bool isValid()
  {
    return true;
  }
};

class ComputePipeline : public GpuPipeline
{
  friend class GpuDevice;
  friend class GraphicsCmdBuffer;
  ComputePipeline(PipelineImpl pipeline) : GpuPipeline(std::forward<decltype(pipeline)>(pipeline)) {}
};

class GraphicsPipeline : public GpuPipeline
{
  friend class GpuDevice;
  friend class GraphicsCmdBuffer;
  GraphicsPipeline(PipelineImpl pipeline) : GpuPipeline(std::forward<decltype(pipeline)>(pipeline)) {}
};
