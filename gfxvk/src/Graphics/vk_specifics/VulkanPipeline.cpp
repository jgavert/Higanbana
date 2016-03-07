#include "VulkanPipeline.hpp"

VulkanPipeline::VulkanPipeline(FazPtrVk<vk::Pipeline> pipeline)
  :m_pipeline(pipeline)
{

}

bool VulkanPipeline::isValid()
{
  return m_pipeline.isValid();
}