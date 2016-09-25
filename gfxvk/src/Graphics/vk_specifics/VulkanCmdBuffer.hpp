#pragma once
#include "gfxvk/src/Graphics/CommandListBasic.hpp"

#include "VulkanBuffer.hpp"
#include "VulkanPipeline.hpp"

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
  ~VulkanCmdBuffer()
  {
    m_cmdBuffer.reset();
    m_pool.reset();
    m_commandList->hardClear();
  }
  // Binding!?!?!?!?, hau, needs pipeline, needs binding.
  void bindComputePipeline(VulkanPipeline& pipeline);

  // copy
  void copy(VulkanBuffer& src, VulkanBuffer& dst);
  // compute
  void dispatch(unsigned x, unsigned y, unsigned z);
  // draw

  bool isValid();
  void close();
  bool isClosed();
};

using CmdBufferImpl = VulkanCmdBuffer;
