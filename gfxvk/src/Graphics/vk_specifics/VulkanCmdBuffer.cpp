#include "VulkanCmdBuffer.hpp"
#include "VulkanGpuDevice.hpp"
#include "core/src/global_debug.hpp"

#include <utility>

#define COMMANDBUFFERSIZE 500*1024
using namespace faze;

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


////////////////////////////////////////////////////////////////////////////////////////
//////////////////// PACKETS START /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

class PipelineBarrierPacket : public VulkanCommandPacket
{
public:
  vk::PipelineStageFlags srcStageMask;
  vk::PipelineStageFlags dstStageMask;
  vk::DependencyFlags dependencyFlags;
  CommandListVector<vk::MemoryBarrier> memoryBarriers;
  CommandListVector<vk::BufferMemoryBarrier> bufferBarriers;
  CommandListVector<vk::ImageMemoryBarrier> imageBarriers;

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
public:
  std::shared_ptr<vk::Buffer> src;
  std::shared_ptr<vk::Buffer> dst;
  CommandListVector<vk::BufferCopy> m_copyList;

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
public:
  std::shared_ptr<vk::Pipeline> pipeline;
  std::shared_ptr<vk::PipelineLayout> layout;
  std::shared_ptr<vk::DescriptorSetLayout> descriptorLayout;
  vk::PipelineBindPoint point;
  BindPipelinePacket(LinearAllocator&, vk::PipelineBindPoint point
    , std::shared_ptr<vk::Pipeline>& pipeline, std::shared_ptr<vk::PipelineLayout> layout
    , std::shared_ptr<vk::DescriptorSetLayout> descriptorLayout)
    : pipeline(pipeline)
    , layout(layout)
    , descriptorLayout(descriptorLayout)
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


class DispatchPacket : public VulkanCommandPacket
{
public:
  VulkanBindingInformation descriptors;
  unsigned dx = 0;
  unsigned dy = 0;
  unsigned dz = 0;
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


////////////////////////////////////////////////////////////////////////////////////////
//////////////////// PACKETS END ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

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
  m_commandList->insert<BindPipelinePacket>(vk::PipelineBindPoint::eCompute, pipeline.m_pipeline, pipeline.m_pipelineLayout, pipeline.m_descriptorSetLayout);
}

// MISC

VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer))
  , m_pool(std::forward<decltype(pool)>(pool))
  , m_closed(false)
  , m_commandList(std::make_shared<CommandList<VulkanCommandPacket>>(LinearAllocator(COMMANDBUFFERSIZE)))
{}

bool VulkanCmdBuffer::isValid()
{
  return m_cmdBuffer.get() != nullptr;
}

bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}

void VulkanCmdBuffer::prepareForSubmit(VulkanGpuDevice& device, VulkanDescriptorPool& pool)
{
  processBindings(device, pool);
}

////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Process Packets ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void VulkanCmdBuffer::close()
{
  m_cmdBuffer->begin(vk::CommandBufferBeginInfo()
    .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    .setPInheritanceInfo(nullptr));

  vk::PipelineLayout pipelineLayout;
  int drawOrDispatch = 0;
  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::BindPipeline:
    {
      BindPipelinePacket* current = static_cast<BindPipelinePacket*>(packet);
      pipelineLayout = *current->layout;
      break;
    }
    case VulkanCommandPacket::PacketType::Dispatch:
    {
      constexpr uint32_t firstSet = 0;
      vk::ArrayProxy<const vk::DescriptorSet> sets(m_updatedSetsPerDraw[drawOrDispatch]);
      vk::ArrayProxy<const uint32_t> setOffsets(0, 0);
      m_cmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, firstSet, sets, setOffsets);
      drawOrDispatch++;
      break;
    }
    default:
      break;
    }
    packet->execute(*m_cmdBuffer);
  });


  m_cmdBuffer->end();
  m_closed = true;
}

void VulkanCmdBuffer::processBindings(VulkanGpuDevice& device, VulkanDescriptorPool& pool)
{
  vk::DescriptorSetLayout* descriptorLayout;

  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::BindPipeline:
    {
      BindPipelinePacket* current = static_cast<BindPipelinePacket*>(packet);
      descriptorLayout = current->descriptorLayout.get();
      break;
    }
    case VulkanCommandPacket::PacketType::Dispatch:
      {
        // handle binding here, create function that does it generically.
        DispatchPacket* dis = static_cast<DispatchPacket*>(packet);
        auto& bind = dis->descriptors;

        auto result = device.m_device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
          .setDescriptorPool(pool.pool)
          .setDescriptorSetCount(1)
          .setPSetLayouts(descriptorLayout));

        std::vector<vk::WriteDescriptorSet> allSets;
        for (auto&& it : bind.buffers)
        {
          allSets.push_back(vk::WriteDescriptorSet()
            .setDescriptorCount(1)
            .setDescriptorType(it.second.type())
            .setDstBinding(it.first)
            .setDstSet(result[0])
            .setPBufferInfo(&it.second.info()));
        }
        vk::ArrayProxy<const vk::WriteDescriptorSet> proxy(allSets);
        device.m_device->updateDescriptorSets(proxy, {});
        
        m_updatedSetsPerDraw.push_back(result[0]);
        break;
      }
    default:
      break;
    }
  });
}

void VulkanCmdBuffer::dependencyFuckup()
{
}

