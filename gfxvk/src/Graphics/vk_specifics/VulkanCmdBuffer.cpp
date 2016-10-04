#include "VulkanCmdBuffer.hpp"

#include "core/src/global_debug.hpp"

#include <utility>

#define COMMANDBUFFERSIZE 500*1024
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

  PacketType type() override
  {
    return PacketType::BufferCopy;
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

  PacketType type() override
  {
    return PacketType::BindPipeline;
  }
};
/*
struct InputDescriptorSetContents
{
  vk::PipelineBindPoint point;
  vk::PipelineLayout layout;
  MemView<vk::DescriptorSet> sets;
  MemView<uint32_t> dynamicOffsets;
};*/

class VulkanBindingInformation
{
public:
  CommandListVector<std::pair< unsigned, VulkanBufferShaderView >> buffers;
  CommandListVector<std::pair< unsigned, VulkanTextureShaderView>> textures;

  VulkanBindingInformation(LinearAllocator& allocator, VulkanDescriptorSet& set)
    : buffers(MemView<std::pair<unsigned, VulkanBufferShaderView>>(allocator.allocList<std::pair<unsigned, VulkanBufferShaderView>>(set.buffers.size()), set.buffers.size()))
    , textures(MemView<std::pair<unsigned, VulkanTextureShaderView>>(allocator.allocList<std::pair<unsigned, VulkanTextureShaderView>>(set.textures.size()), set.textures.size()))
  {
    for (size_t i = 0; i < set.buffers.size(); i++)
    {
      buffers[i] = set.buffers[i];
    }
    for (size_t i = 0; i < set.textures.size(); i++)
    {
      textures[i] = set.textures[i];
    }
  }
};
/*
class DescriptorSetContents
{
  vk::PipelineBindPoint point;
  vk::PipelineLayout layout;
  CommandListVector<vk::DescriptorSet> descriptors;
  CommandListVector<uint32_t> dynOffsets;
public:
  DescriptorSetContents(LinearAllocator& allocator, InputDescriptorSetContents input)
    : point(input.point)
    , layout(input.layout)
    , descriptors(MemView<vk::DescriptorSet>(allocator.allocList<vk::DescriptorSet>(input.sets.size()), input.sets.size()))
    , dynOffsets(MemView<uint32_t>(allocator.allocList<uint32_t>(input.dynamicOffsets.size()), input.dynamicOffsets.size()))
  {
    for (size_t i = 0; i < input.sets.size(); i++)
    {
      descriptors[i] = input.sets[i];
    }
    for (size_t i = 0; i < input.dynamicOffsets.size(); i++)
    {
      dynOffsets[i] = input.dynamicOffsets[i];
    }
  }

  void bind(vk::CommandBuffer& cmd)
  {
    constexpr uint32_t firstSet = 0;
    vk::ArrayProxy<const vk::DescriptorSet> sets(static_cast<uint32_t>(descriptors.size()), descriptors.data());
    vk::ArrayProxy<const uint32_t> setOffsets(static_cast<uint32_t>(dynOffsets.size()), dynOffsets.data());
    cmd.bindDescriptorSets(point, layout, firstSet, sets, setOffsets);
  }
};
*/


class DispatchPacket : public VulkanCommandPacket
{
private:
  VulkanBindingInformation descriptors;
  unsigned dx = 0;
  unsigned dy = 0;
  unsigned dz = 0;
public:
  DispatchPacket(LinearAllocator& allocator, VulkanDescriptorSet& inputs, unsigned x, unsigned y, unsigned z)
    : descriptors(allocator, std::forward<decltype(inputs)>(inputs))
    , dx(x)
    , dy(y)
    , dz(z)
  {
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    cmd.dispatch(dx, dy, dz);
  }

  PacketType type() override
  {
    return PacketType::Dispatch;
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


void VulkanCmdBuffer::dispatch(VulkanDescriptorSet& set, unsigned x, unsigned y, unsigned z)
{
  m_commandList->insert<DispatchPacket>(set, x, y, z);
}

void VulkanCmdBuffer::bindComputePipeline(VulkanPipeline& pipeline)
{
  m_commandList->insert<BindPipelinePacket>(vk::PipelineBindPoint::eCompute, pipeline.m_pipeline);
}

VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer)), m_pool(std::forward<decltype(pool)>(pool)), m_closed(false)
  , m_commandList(std::make_shared<CommandList<VulkanCommandPacket>>(LinearAllocator(COMMANDBUFFERSIZE)))
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
  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    packet->execute(*m_cmdBuffer);
  });
  m_cmdBuffer->end();
  m_closed = true;
}

bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}

void VulkanCmdBuffer::dependencyFuckup()
{
}

void VulkanCmdBuffer::prepareForSubmit(VulkanGpuDevice& device)
{
  processBindings(device);
}

void VulkanCmdBuffer::processBindings(VulkanGpuDevice& )
{
  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::Dispatch:
    {
      // handle binding here, create function that does it generically.
      break;
    }
    default:
      break;
    }
  });
}