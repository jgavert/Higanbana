#pragma once
#include "gfxvk/src/Graphics/PipelineDescriptor.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>

class VulkanPipeline
{
private:
  friend class VulkanGpuDevice;

  std::shared_ptr<vk::Pipeline>    m_pipeline;
  std::shared_ptr<vk::PipelineLayout> m_pipelineLayout;
  std::shared_ptr<GraphicsPipelineDescriptor> m_graphDesc;
  std::shared_ptr<ComputePipelineDescriptor> m_computeDesc;

  VulkanPipeline() {}
  VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, std::shared_ptr<vk::PipelineLayout> layout, GraphicsPipelineDescriptor graphDesc);
  VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, std::shared_ptr<vk::PipelineLayout> layout, ComputePipelineDescriptor computeDesc);
public:
  GraphicsPipelineDescriptor& graphDesc();
  ComputePipelineDescriptor& computeDesc();
  bool isValid();
};

using PipelineImpl = VulkanPipeline;