#pragma once

#include <memory>
#include <vulkan/vk_cpp.h>

class VulkanCmdBuffer
{
private:
  friend class VulkanGpuDevice;
  friend class VulkanQueue;
  std::shared_ptr<vk::CommandBuffer>   m_cmdBuffer;
  std::shared_ptr<vk::CommandPool>     m_pool;
  bool                          m_closed;
  VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool);
public:
  void resetList();
  bool isValid();
  void closeList();
  bool isClosed();
};

using CmdBufferImpl = VulkanCmdBuffer;
