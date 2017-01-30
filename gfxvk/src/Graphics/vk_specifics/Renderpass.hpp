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
  struct privates
  {

    vk::RenderPass renderpass;
    bool created = false;
  };
  std::shared_ptr<privates> m_priv;
public:
  VulkanRenderpass()
    : m_priv(std::make_shared<privates>())
  {}

  vk::RenderPass& native()
  {
    return m_priv->renderpass;
  }

  bool created()
  {
    return m_priv->created;
  }
};

using RenderpassImpl = VulkanRenderpass;
