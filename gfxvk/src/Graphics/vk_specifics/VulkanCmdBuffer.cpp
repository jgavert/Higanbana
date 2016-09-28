#include "VulkanCmdBuffer.hpp"

#include "core/src/global_debug.hpp"


using namespace faze;

//#undef MemoryBarrier 

class PipelineBarrierPacket : public VulkanCommandPacket
{
private:
  vk::PipelineStageFlags srcStageMask;
  vk::PipelineStageFlags dstStageMask;
  vk::DependencyFlags dependencyFlags;
  CommandListVector<vk::MemoryBarrier> memoryBarriers;
  CommandListVector<vk::BufferMemoryBarrier> bufferBarriers;
  CommandListVector<vk::ImageMemoryBarrier> imageBarriers;
public:

  PipelineBarrierPacket(LinearAllocator& allocator,
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::DependencyFlags dependencyFlags,
    MemView<vk::MemoryBarrier> memoryBarriers,
    MemView<vk::BufferMemoryBarrier> bufferBarriers,
    MemView<vk::ImageMemoryBarrier> imageBarriers)
    : srcStageMask(srcStageMask)
    , dstStageMask(dstStageMask)
    , dependencyFlags(dependencyFlags)
    , memoryBarriers(MemView<vk::MemoryBarrier>(allocator.allocList<vk::MemoryBarrier>(memoryBarriers.size()), memoryBarriers.size()))
    , bufferBarriers(MemView<vk::BufferMemoryBarrier>(allocator.allocList<vk::BufferMemoryBarrier>(bufferBarriers.size()), bufferBarriers.size()))
    , imageBarriers(MemView<vk::ImageMemoryBarrier>(allocator.allocList<vk::ImageMemoryBarrier>(imageBarriers.size()), imageBarriers.size()))
  {

  }
  void execute(vk::CommandBuffer& cmd) override
  {
    vk::ArrayProxy<const vk::MemoryBarrier> memory(static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data());
    vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data());
    vk::ArrayProxy<const vk::ImageMemoryBarrier> image(static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
    cmd.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memory, buffer, image);
  }
};

class BufferCopyPacket : public VulkanCommandPacket
{
private:
  std::shared_ptr<vk::Buffer> src;
  std::shared_ptr<vk::Buffer> dst;
  CommandListVector<vk::BufferCopy> m_copyList;
public:

  BufferCopyPacket(LinearAllocator& allocator, std::shared_ptr<vk::Buffer> src, std::shared_ptr<vk::Buffer> dst, MemView<vk::BufferCopy> copyList)
    : src(src)
    , dst(dst)
    , m_copyList(MemView<vk::BufferCopy>(allocator.allocList<vk::BufferCopy>(copyList.size()), copyList.size()))
  {
    F_ASSERT(copyList, "Invalid copy commands");
    for (size_t i = 0; i < m_copyList.size(); i++)
    {
      m_copyList[i] = copyList[i];
    }
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    vk::ArrayProxy<const vk::BufferCopy> proxy(static_cast<uint32_t>(m_copyList.size()), m_copyList.data());
    cmd.copyBuffer(*src, *dst, proxy);
  }
};

class BindPipelinePacket : public VulkanCommandPacket
{
private:
  std::shared_ptr<vk::Pipeline> pipeline;
  vk::PipelineBindPoint point;
public:
  BindPipelinePacket(LinearAllocator&, vk::PipelineBindPoint point, std::shared_ptr<vk::Pipeline>& pipeline)
    : pipeline(pipeline)
    , point(point)
  {
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    cmd.bindPipeline(point, *pipeline);
  }
};

class DispatchPacket : public VulkanCommandPacket
{
private:
  unsigned dx = 0;
  unsigned dy = 0;
  unsigned dz = 0;
public:
  DispatchPacket(LinearAllocator& , unsigned x, unsigned y, unsigned z)
    : dx(x)
    , dy(y)
    , dz(z)
  {
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    cmd.dispatch(dx, dy, dz);
  }
};

class BindDescriptorSetPacket : public VulkanCommandPacket
{
private:
  vk::PipelineBindPoint point;
  vk::PipelineLayout layout;
  CommandListVector<vk::DescriptorSet> descriptors;
  CommandListVector<uint32_t> dynOffsets;
public:
  BindDescriptorSetPacket(LinearAllocator& allocator, vk::PipelineBindPoint point, vk::PipelineLayout layout
                        , MemView<vk::DescriptorSet> sets, MemView<uint32_t> dynamicOffsets)
    : point(point)
    , layout(layout)
    , descriptors(MemView<vk::DescriptorSet>(allocator.allocList<vk::DescriptorSet>(sets.size()), sets.size()))
    , dynOffsets(MemView<uint32_t>(allocator.allocList<uint32_t>(dynamicOffsets.size()), dynamicOffsets.size()))
  {
    for (size_t i = 0; i < sets.size(); i++)
    {
      descriptors[i] = sets[i];
    }
    for (size_t i = 0; i < dynamicOffsets.size(); i++)
    {
      dynOffsets[i] = dynamicOffsets[i];
    }
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    // using only 1 set
    constexpr uint32_t firstSet = 0;
    vk::ArrayProxy<const vk::DescriptorSet> sets(static_cast<uint32_t>(descriptors.size()), descriptors.data());
    vk::ArrayProxy<const uint32_t> setOffsets(static_cast<uint32_t>(dynOffsets.size()), dynOffsets.data());
    cmd.bindDescriptorSets(point, layout, firstSet, sets, setOffsets);
  }
};


void VulkanCmdBuffer::copy(VulkanBuffer& src, VulkanBuffer& dst)
{
  vk::BufferCopy copy;
  auto srcSize = src.desc().m_stride * src.desc().m_width;
  auto dstSize = dst.desc().m_stride * dst.desc().m_width;
  auto maxSize = std::min(srcSize, dstSize);
  copy = copy.setSize(maxSize)
    .setDstOffset(0)
    .setSrcOffset(0);
  m_commandList->insert<BufferCopyPacket>(src.m_resource, dst.m_resource, copy);
}

void VulkanCmdBuffer::bindComputeDescriptorSet(VulkanPipeline& pipeline, VulkanDescriptorSet& set)
{
  m_commandList->insert<BindDescriptorSetPacket>(vk::PipelineBindPoint::eCompute, *pipeline.m_pipelineLayout, set.set, MemView<uint32_t>());
}

void VulkanCmdBuffer::dispatch(unsigned x, unsigned y, unsigned z)
{
  m_commandList->insert<DispatchPacket>(x, y, z);
}

void VulkanCmdBuffer::bindComputePipeline(VulkanPipeline& pipeline)
{
  m_commandList->insert<BindPipelinePacket>(vk::PipelineBindPoint::eCompute, pipeline.m_pipeline);
}

VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer)), m_pool(std::forward<decltype(pool)>(pool)), m_closed(false)
  , m_commandList(std::make_shared<CommandList<VulkanCommandPacket>>(LinearAllocator(1024*512)))
{}

bool VulkanCmdBuffer::isValid()
{
  return m_cmdBuffer.get() != nullptr;
}
void VulkanCmdBuffer::close()
{
  m_cmdBuffer->begin(vk::CommandBufferBeginInfo()
    .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    .setPInheritanceInfo(nullptr));
  m_commandList->foreach([&](VulkanCommandPacket& packet)
  {
    packet.execute(*m_cmdBuffer);
  });
  m_cmdBuffer->end();
  m_closed = true;
}

bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}
