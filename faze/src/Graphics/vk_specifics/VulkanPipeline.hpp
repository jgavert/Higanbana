#pragma once
#include "faze/src/Graphics/PipelineDescriptor.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>

class VulkanPipeline
{
private:
  friend class VulkanGpuDevice;
  friend class VulkanCmdBuffer;
  friend class VulkanDescriptorSet;

  std::shared_ptr<vk::Pipeline>    m_pipeline;
  std::shared_ptr<vk::PipelineLayout> m_pipelineLayout;
  std::shared_ptr<vk::DescriptorSetLayout> m_descriptorSetLayout;
  std::shared_ptr<GraphicsPipelineDescriptor> m_graphDesc;
  std::shared_ptr<ComputePipelineDescriptor> m_computeDesc;

  VulkanPipeline() {}
  VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, std::shared_ptr<vk::PipelineLayout> layout, std::shared_ptr<vk::DescriptorSetLayout> layoutSet, GraphicsPipelineDescriptor graphDesc);
  VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, std::shared_ptr<vk::PipelineLayout> layout, std::shared_ptr<vk::DescriptorSetLayout> layoutSet, ComputePipelineDescriptor computeDesc);
public:
  GraphicsPipelineDescriptor& graphDesc();
  ComputePipelineDescriptor& computeDesc();
  bool isValid();
};

using PipelineImpl = VulkanPipeline;