#pragma once
#include "core/src/memory/ManagedResource.hpp"
#include <vulkan/vk_cpp.h>

class VulkanPipeline
{
private:
  FazPtrVk<vk::Pipeline>    m_pipeline;
  VulkanPipeline(FazPtrVk<vk::Pipeline> queue);
public:
  bool isValid();
};

using PipelineImpl = VulkanPipeline;