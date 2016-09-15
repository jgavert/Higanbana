#include "VulkanPipeline.hpp"


VulkanPipeline::VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, std::shared_ptr<vk::PipelineLayout> layout, GraphicsPipelineDescriptor graphDesc)
  : m_pipeline(pipeline)
  , m_pipelineLayout(layout)
  , m_graphDesc(std::make_shared<decltype(graphDesc)>(std::forward<decltype(graphDesc)>(graphDesc)))
{

}
VulkanPipeline::VulkanPipeline(std::shared_ptr<vk::Pipeline> pipeline, std::shared_ptr<vk::PipelineLayout> layout, ComputePipelineDescriptor computeDesc)
  : m_pipeline(pipeline)
  , m_pipelineLayout(layout)
  , m_computeDesc(std::make_shared<decltype(computeDesc)>(std::forward<decltype(computeDesc)>(computeDesc)))
{

}

GraphicsPipelineDescriptor& VulkanPipeline::graphDesc()
{
	return *m_graphDesc;
}

ComputePipelineDescriptor& VulkanPipeline::computeDesc()
{
	return *m_computeDesc;
}

bool VulkanPipeline::isValid()
{
  return m_pipeline.get() != nullptr;
}