#pragma once
#include "gfxvk/src/Graphics/CommandListBasic.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

class VulkanCommandPacket
{
private:
  VulkanCommandPacket* m_nextPacket;
public:
  VulkanCommandPacket()
    :m_nextPacket(nullptr)
  {}

  void setNextPacket(VulkanCommandPacket* packet)
  {
    m_nextPacket = packet;
  }
  VulkanCommandPacket* nextPacket()
  {
    return m_nextPacket;
  }

  virtual void execute(vk::CommandBuffer& buffer) = 0;
  virtual ~VulkanCommandPacket() {}
};

class VulkanBuffer;
class VulkanTexture;

class VulkanCmdBuffer
{
private:
  friend class VulkanGpuDevice;
  friend class VulkanQueue;
  std::shared_ptr<vk::CommandBuffer>   m_cmdBuffer;
  std::shared_ptr<vk::CommandPool>     m_pool;
  bool                                 m_closed = false;
  std::shared_ptr<CommandList<VulkanCommandPacket>>     m_commandList;
  VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool);
public:
  VulkanCmdBuffer() {}
  // Binding!?!?!?!?, hau, needs pipeline, needs binding.

  // copy
  void copy(VulkanBuffer src, VulkanBuffer dst);
  // compute
  void dispatch();
  // draw

  bool isValid();
  void close();
  bool isClosed();
};

using CmdBufferImpl = VulkanCmdBuffer;
