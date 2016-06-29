#pragma once
#include "gfxvk/src/Graphics/PipelineDescriptor.hpp"
#include <vulkan/vk_cpp.h>
#include <memory>

class VulkanPipeline
{
private:
  friend class VulkanGpuDevice;

  std::shared_ptr<vk::Pipeline>    m_pipeline;
  std::shared_ptr<GraphicsPipelineDescriptor> m_graphDesc;
  std::shared_ptr<ComputePipelineDescriptor> m_computeDesc;

  VulkanPipeline() {}
  VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, GraphicsPipelineDescriptor graphDesc);
  VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, ComputePipelineDescriptor computeDesc);
public:
  GraphicsPipelineDescriptor& graphDesc();
  ComputePipelineDescriptor& computeDesc();
  bool isValid();
};

using PipelineImpl = VulkanPipeline;