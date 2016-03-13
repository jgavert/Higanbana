#include "VulkanPipeline.hpp"


VulkanPipeline::VulkanPipeline(FazPtrVk<vk::Pipeline> pipeline, GraphicsPipelineDescriptor graphDesc)
  : m_pipeline(pipeline)
  , m_graphDesc(std::forward<decltype(graphDesc)>(graphDesc))
{

}
VulkanPipeline::VulkanPipeline(FazPtrVk<vk::Pipeline> pipeline, ComputePipelineDescriptor computeDesc)
  : m_pipeline(pipeline)
  , m_computeDesc(std::forward<decltype(computeDesc)>(computeDesc))
{

}

bool VulkanPipeline::isValid()
{
  return m_pipeline.isValid();
}