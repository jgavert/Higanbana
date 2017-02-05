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

  std::shared_ptr<std::shared_ptr<vk::RenderPass>> m_renderpass;


public:
  VulkanRenderpass()
    : m_renderpass(std::make_shared<std::shared_ptr<vk::RenderPass>>(nullptr))
  {}

  void create(std::shared_ptr<vk::RenderPass> rp)
  {
    *m_renderpass = rp;
  }

  vk::RenderPass& native()
  {
    return *(*m_renderpass);
  }

  bool created()
  {
    return (*m_renderpass).get() != nullptr;
  }
};

using RenderpassImpl = VulkanRenderpass;
