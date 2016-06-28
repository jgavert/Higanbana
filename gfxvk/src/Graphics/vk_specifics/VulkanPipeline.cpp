#include "VulkanPipeline.hpp"


VulkanPipeline::VulkanPipeline(FazPtrVk<vk::Pipeline> pipeline, GraphicsPipelineDescriptor graphDesc)
  : m_pipeline(pipeline)
  , m_graphDesc(std::make_shared<decltype(graphDesc)>(std::forward<decltype(graphDesc)>(graphDesc)))
{

}
VulkanPipeline::VulkanPipeline(FazPtrVk<vk::Pipeline> pipeline, ComputePipelineDescriptor computeDesc)
  : m_pipeline(pipeline)
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
  return m_pipeline.isValid();
}