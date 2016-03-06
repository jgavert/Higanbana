#pragma once
#include "core/src/memory/ManagedResource.hpp"

#include <vulkan/vk_cpp.h>

class VulkanCmdBuffer
{
private:
  friend class VulkanGpuDevice;
  friend class VulkanQueue;
  FazPtrVk<vk::CommandBuffer>   m_cmdBuffer;
  bool                          m_closed;
  VulkanCmdBuffer(FazPtrVk<vk::CommandBuffer> buffer);
public:
  void resetList();
  bool isValid();
  void closeList();
  bool isClosed();
};

using CmdBufferImpl = VulkanCmdBuffer;
