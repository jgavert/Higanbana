#pragma once

#include <vulkan/vulkan.hpp>

class VulkanSubpass
{
private:

public:

};

using SubpassImpl = VulkanSubpass;

class VulkanRenderpass
{
private:
  vk::RenderPass renderpass;
  bool created = false;
public:

};

using RenderpassImpl = VulkanRenderpass;
