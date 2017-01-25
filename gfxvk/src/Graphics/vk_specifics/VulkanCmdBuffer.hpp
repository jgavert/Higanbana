#pragma once
#include "gfxvk/src/Graphics/CommandListBasic.hpp"

#include "VulkanBuffer.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanDescriptorSet.hpp"
#include "VulkanDescriptorPool.hpp"
#include "VulkanDependency.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

struct DependencyInfoBuffer
{
  vk::Buffer buffer;
  vk::DeviceSize offset;
  vk::DeviceSize size;
  vk::AccessFlags access;
};

class VulkanCommandPacket
{
private:
  VulkanCommandPacket* m_nextPacket;
public:

  enum class PacketType
  {
    PipelineBarrier,
    BindPipeline,
    BufferCopy,
	ClearRTV,
    Dispatch,
	PrepareForPresent,
	RenderpassBegin,
	RenderpassEnd,
	SubpassBegin,
	SubpassEnd
  };

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
  virtual PacketType type() = 0;
  virtual ~VulkanCommandPacket() {}
};

class VulkanBuffer;
class VulkanTexture;
class VulkanGpuDevice;

class VulkanCmdBuffer
{
private:
  friend class VulkanGpuDevice;
  friend class VulkanQueue;
  std::shared_ptr<vk::CommandBuffer>   m_cmdBuffer;
  std::shared_ptr<vk::CommandPool>     m_pool;
  bool                                 m_closed = false;
  std::shared_ptr<CommandList<VulkanCommandPacket>>     m_commandList;
  std::vector<vk::DescriptorSet>      m_updatedSetsPerDraw;
  DependencyTracker					tracker;

  // tempdata

  std::vector<vk::WriteDescriptorSet> m_allSets;
  std::vector<vk::DescriptorSetLayout> m_layouts;

  VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool);
public:
  VulkanCmdBuffer() {}
  ~VulkanCmdBuffer()
  {
    m_commandList->hardClear();
    m_cmdBuffer.reset();
    m_pool.reset();
  }
  // supported commands
  void bindComputePipeline(VulkanPipeline& pipeline);
  void dispatch(VulkanDescriptorSet& set, unsigned x, unsigned y, unsigned z);
  void copy(VulkanBuffer& src, VulkanBuffer& dst);
  void clearRTV(VulkanTexture& texture, float r, float g, float b, float a);
  void prepareForPresent(VulkanTexture& texture);

  // misc
  bool isValid();
  void close();
  bool isClosed();

  // renderpass
  void beginRenderpass();
  void endRenderpass();
  void beginSubpass();
  void endSubpass();

  // Call before submit
  void prepareForSubmit(VulkanGpuDevice& device, VulkanDescriptorPool& pool);
private:
  void processRenderpasses(VulkanGpuDevice& device);
  void processBindings(VulkanGpuDevice& device, VulkanDescriptorPool& pool);
  void dependencyFuckup();
};

using CmdBufferImpl = VulkanCmdBuffer;
