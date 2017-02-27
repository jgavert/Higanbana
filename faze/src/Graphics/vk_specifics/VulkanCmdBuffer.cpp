#include "VulkanCmdBuffer.hpp"
#include "VulkanGpuDevice.hpp"
#include "core/src/global_debug.hpp"

#include <utility>
#include <deque>


#define COMMANDBUFFERSIZE 500*1024
using namespace faze;

class VulkanBindingInformation
{
public:
  CommandListVector<std::pair< unsigned, VulkanBufferShaderView >> readBuffers;
  CommandListVector<std::pair< unsigned, VulkanTextureShaderView>> readTextures;
  CommandListVector<std::pair< unsigned, VulkanBufferShaderView >> modifyBuffers;
  CommandListVector<std::pair< unsigned, VulkanTextureShaderView>> modifyTextures;

  VulkanBindingInformation(LinearAllocator& allocator, VulkanDescriptorSet& set)
    : readBuffers(MemView<std::pair<unsigned, VulkanBufferShaderView>>(allocator.allocList<std::pair<unsigned, VulkanBufferShaderView>>(set.readBuffers.size()), set.readBuffers.size()))
    , readTextures(MemView<std::pair<unsigned, VulkanTextureShaderView>>(allocator.allocList<std::pair<unsigned, VulkanTextureShaderView>>(set.readTextures.size()), set.readTextures.size()))
    , modifyBuffers(MemView<std::pair<unsigned, VulkanBufferShaderView>>(allocator.allocList<std::pair<unsigned, VulkanBufferShaderView>>(set.modifyBuffers.size()), set.modifyBuffers.size()))
    , modifyTextures(MemView<std::pair<unsigned, VulkanTextureShaderView>>(allocator.allocList<std::pair<unsigned, VulkanTextureShaderView>>(set.modifyTextures.size()), set.modifyTextures.size()))
  {
    for (size_t i = 0; i < set.readBuffers.size(); i++)
    {
      readBuffers[i] = set.readBuffers[i];
    }
    for (size_t i = 0; i < set.readTextures.size(); i++)
    {
      readTextures[i] = set.readTextures[i];
    }
    for (size_t i = 0; i < set.modifyBuffers.size(); i++)
    {
      modifyBuffers[i] = set.modifyBuffers[i];
    }
    for (size_t i = 0; i < set.modifyTextures.size(); i++)
    {
      modifyTextures[i] = set.modifyTextures[i];
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
  VulkanBuffer src;
  VulkanBuffer dst;
  CommandListVector<vk::BufferCopy> m_copyList;

  BufferCopyPacket(LinearAllocator& allocator, VulkanBuffer src, VulkanBuffer dst, MemView<vk::BufferCopy> copyList)
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
    cmd.copyBuffer(src.impl(), dst.impl(), proxy);
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

class PrepareForPresentPacket : public VulkanCommandPacket
{
public:
	VulkanTexture texture;

	PrepareForPresentPacket(LinearAllocator&, VulkanTexture& texture)
		:texture(texture)
	{
	}

	void execute(vk::CommandBuffer& ) override
	{
	}

	PacketType type() override
	{
		return PacketType::PrepareForPresent;
	}
};

class ClearRTVPacket : public VulkanCommandPacket
{
public:
	VulkanTexture texture;
	vk::ClearColorValue clearValue;

	ClearRTVPacket(LinearAllocator&, VulkanTexture& texture, vk::ClearColorValue clearValue)
		:texture(texture)
		, clearValue(clearValue)
	{
	}

	void execute(vk::CommandBuffer& cmd) override
	{
		vk::ImageSubresourceRange range = vk::ImageSubresourceRange()
			.setBaseMipLevel(0)
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setLevelCount(1);
		cmd.clearColorImage(texture.impl(), vk::ImageLayout::eGeneral, clearValue, range);
	}

	PacketType type() override
	{
		return PacketType::ClearRTV;
	}
};

class RenderpassBeginPacket : public VulkanCommandPacket
{
public:
  VulkanRenderpass rp;

  VulkanTextureShaderView rtv;

	RenderpassBeginPacket(LinearAllocator&, VulkanRenderpass rp, VulkanTextureShaderView rtv)
    : rp(rp)
    , rtv(rtv)
	{
	}

	void execute(vk::CommandBuffer& ) override
	{
	
	}

	PacketType type() override
	{
		return PacketType::RenderpassBegin;
	}
};

class RenderpassEndPacket : public VulkanCommandPacket
{
public:

	RenderpassEndPacket(LinearAllocator&)
	{
	}

	void execute(vk::CommandBuffer&) override
	{

	}

	PacketType type() override
	{
		return PacketType::RenderpassEnd;
	}
};

class SubpassBeginPacket : public VulkanCommandPacket
{
public:

	SubpassBeginPacket(LinearAllocator&)
	{
	}

	void execute(vk::CommandBuffer&) override
	{

	}

	PacketType type() override
	{
		return PacketType::SubpassBegin;
	}
};

class SubpassEndPacket : public VulkanCommandPacket
{
public:

	SubpassEndPacket(LinearAllocator&)
	{
	}

	void execute(vk::CommandBuffer&) override
	{

	}

	PacketType type() override
	{
		return PacketType::SubpassEnd;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
//////////////////// PACKETS END ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void VulkanCmdBuffer::bindComputePipeline(VulkanPipeline& pipeline)
{
	m_commandList->insert<BindPipelinePacket>(vk::PipelineBindPoint::eCompute, pipeline.m_pipeline, pipeline.m_pipelineLayout, pipeline.m_descriptorSetLayout);
}

void VulkanCmdBuffer::dispatch(VulkanDescriptorSet& set, unsigned x, unsigned y, unsigned z)
{
  m_commandList->insert<DispatchPacket>(set, x, y, z);
}

void VulkanCmdBuffer::copy(VulkanBuffer& src, VulkanBuffer& dst)
{
	vk::BufferCopy copy;
	auto srcSize = src.resourceSize;
	auto dstSize = dst.resourceSize;
	auto maxSize = (std::min)(srcSize, dstSize);
	copy = copy.setSize(maxSize)
		.setDstOffset(0)
		.setSrcOffset(0);
	m_commandList->insert<BufferCopyPacket>(src, dst, copy);
}

void VulkanCmdBuffer::clearRTV(VulkanTexture& texture, float r, float g, float b, float a)
{
	m_commandList->insert<ClearRTVPacket>(texture, vk::ClearColorValue().setFloat32({ r, g, b, a }));

}

void VulkanCmdBuffer::prepareForPresent(VulkanTexture& texture)
{
	m_commandList->insert<PrepareForPresentPacket>(texture);
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
  processRenderpasses(device);
  processBindings(device, pool);
  dependencyFuckup();
}
////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Renderpass ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void VulkanCmdBuffer::beginRenderpass(VulkanRenderpass& rp, VulkanTextureShaderView rtv)
{
  m_commandList->insert<RenderpassBeginPacket>(rp, rtv);
}

void VulkanCmdBuffer::endRenderpass()
{
  m_commandList->insert<RenderpassEndPacket>();
}

void VulkanCmdBuffer::beginSubpass()
{
	m_commandList->insert<SubpassBeginPacket>();
}

void VulkanCmdBuffer::endSubpass()
{
	m_commandList->insert<SubpassEndPacket>();
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
  int barrierCall = 0;
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
	case VulkanCommandPacket::PacketType::BufferCopy:
	{
		tracker.runBarrier(*m_cmdBuffer, barrierCall++);
		break;
	}
    case VulkanCommandPacket::PacketType::Dispatch:
    {
	  tracker.runBarrier(*m_cmdBuffer, barrierCall++);
      constexpr uint32_t firstSet = 0;
      vk::ArrayProxy<const vk::DescriptorSet> sets(m_updatedSetsPerDraw[drawOrDispatch]);
      vk::ArrayProxy<const uint32_t> setOffsets(0, 0);
      m_cmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, firstSet, sets, setOffsets);
      drawOrDispatch++;
      break;
    }

    case VulkanCommandPacket::PacketType::PrepareForPresent:
    {
      tracker.runBarrier(*m_cmdBuffer, barrierCall++);
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

void VulkanCmdBuffer::processRenderpasses(VulkanGpuDevice& device)
{
	int unhandledRenderpasses = 0;
	bool insideRenderpass = false;
  RenderpassBeginPacket* active = nullptr;

	m_commandList->foreach([&](VulkanCommandPacket* packet)
	{
		switch (packet->type())
		{
		case VulkanCommandPacket::PacketType::RenderpassBegin:
      active = static_cast<RenderpassBeginPacket*>(packet);
			insideRenderpass = true;
			break;
		case VulkanCommandPacket::PacketType::RenderpassEnd:
      if (insideRenderpass)
      {
        if (!active->rp.created())
        {
          unhandledRenderpasses++;

          // todo: these attachments should be defined from usercode, not hardcoded.
          vk::AttachmentDescription attachment = vk::AttachmentDescription()
            .setInitialLayout(active->rtv.info().imageLayout)
            .setFinalLayout(active->rtv.info().imageLayout)
            .setFormat(active->rtv.format())
            .setLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

          vk::ArrayProxy<const vk::AttachmentDescription> attachments(attachment);

          vk::AttachmentReference refRtv = vk::AttachmentReference()
            .setAttachment(0) // hardcoded.
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
 
          vk::ArrayProxy<const vk::AttachmentReference> colorAttachments(refRtv);

          uint32_t preservedAttachments[1] = { 0 };

          vk::SubpassDescription subpass = vk::SubpassDescription()
            .setColorAttachmentCount(1)
            .setPColorAttachments(colorAttachments.data())
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setPreserveAttachmentCount(1)
            .setPPreserveAttachments(preservedAttachments);

          vk::ArrayProxy<const vk::SubpassDescription> subpasses(subpass);

          vk::RenderPassCreateInfo info = vk::RenderPassCreateInfo()
            .setAttachmentCount(1)
            .setPAttachments(attachments.data())
            .setDependencyCount(0)
            .setSubpassCount(1)
            .setPSubpasses(subpasses.data());

          auto rp = device.native().createRenderPass(info);

          auto dev = device.native();

          active->rp.create(std::shared_ptr<vk::RenderPass>(new vk::RenderPass(rp), [dev](vk::RenderPass* ptr)
          {
            dev.destroyRenderPass(*ptr);
            delete ptr;
          }));
        }
      }
			insideRenderpass = false;
			break;
		default:
			break;
		}
	});
  if (unhandledRenderpasses > 0)
	  F_LOG("Renderpasses to compile: %d\n", unhandledRenderpasses);
}

void VulkanCmdBuffer::processBindings(VulkanGpuDevice& device, VulkanDescriptorPool& pool)
{
  m_layouts.clear();
  m_allSets.clear();
  m_updatedSetsPerDraw.clear();
  //m_layouts.reserve(m_commandList->size()/2);
  //m_allSets.reserve(m_commandList->size()*10);
  //m_updatedSetsPerDraw.reserve(m_layouts.size());
  // count layouts
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
      m_layouts.emplace_back(*descriptorLayout);
      break;
    }
    default:
      break;
    }
  });

  if (!m_layouts.empty())
  {
    m_updatedSetsPerDraw = device.m_device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(pool.pool)
      .setDescriptorSetCount(static_cast<uint32_t>(m_layouts.size()))
      .setPSetLayouts(m_layouts.data()));
  }

  // make sets
  int setCount = 0;
  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::BindPipeline:
    {
      break;
    }
    case VulkanCommandPacket::PacketType::Dispatch:
      {
        // handle binding here, create function that does it in a generic way.
        DispatchPacket* dis = static_cast<DispatchPacket*>(packet);
        auto& bind = dis->descriptors;
        for (auto&& it : bind.readBuffers)
        {
          m_allSets.emplace_back(vk::WriteDescriptorSet()
            .setDescriptorCount(1)
            .setDescriptorType(it.second.type())
            .setDstBinding(it.first)
            .setDstSet(m_updatedSetsPerDraw[setCount])
            .setPBufferInfo(&it.second.info()));
        }
        for (auto&& it : bind.modifyBuffers)
        {
          m_allSets.emplace_back(vk::WriteDescriptorSet()
            .setDescriptorCount(1)
            .setDescriptorType(it.second.type())
            .setDstBinding(it.first)
            .setDstSet(m_updatedSetsPerDraw[setCount])
            .setPBufferInfo(&it.second.info()));
        }
        ++setCount;
        break;
      }
    default:
      break;
    }
  });

  vk::ArrayProxy<const vk::WriteDescriptorSet> proxy(m_allSets);
  device.m_device->updateDescriptorSets(proxy, {});
}

void VulkanCmdBuffer::dependencyFuckup()
{
  tracker.reset();
  int drawCallIndex = 0;
  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::BufferCopy:
    {
      BufferCopyPacket* p = static_cast<BufferCopyPacket*>(packet);
      F_ASSERT(p->m_copyList.size() == 1, "Dependency tracker doesn't support more than 1 copy.");
      auto first = p->m_copyList[0];

      tracker.addDrawCall(drawCallIndex, DependencyTracker::DrawType::BufferCopy, vk::PipelineStageFlagBits::eTransfer);
      tracker.addReadBuffer(drawCallIndex, p->src, first.srcOffset, first.size, vk::AccessFlagBits::eTransferRead);
      tracker.addModifyBuffer(drawCallIndex, p->dst, first.dstOffset, first.size, vk::AccessFlagBits::eTransferWrite);

      drawCallIndex++;
      break;
    }
    case VulkanCommandPacket::PacketType::Dispatch:
    {
      DispatchPacket* p = static_cast<DispatchPacket*>(packet);
      auto& bind = p->descriptors;

      tracker.addDrawCall(drawCallIndex, DependencyTracker::DrawType::Dispatch, vk::PipelineStageFlagBits::eComputeShader);
      for (auto&& it : bind.readBuffers)
      {
        tracker.addReadBuffer(drawCallIndex, it.second, vk::AccessFlagBits::eShaderRead);
      }
      for (auto&& it : bind.modifyBuffers)
      {
        tracker.addModifyBuffer(drawCallIndex, it.second, vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);
      }
      drawCallIndex++;
      break;
    }
	case VulkanCommandPacket::PacketType::PrepareForPresent:
	{
		PrepareForPresentPacket* p = static_cast<PrepareForPresentPacket*>(packet);
		tracker.addDrawCall(drawCallIndex, DependencyTracker::DrawType::PrepareForPresent, vk::PipelineStageFlagBits::eColorAttachmentOutput);
		tracker.addReadTexture(drawCallIndex, p->texture, vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::ePresentSrcKHR);
		drawCallIndex++;
	}
    default:
      break;
    }
  });

  //tracker.resolveGraph();
  tracker.makeAllBarriers();
  /*
  tracker.printStuff([](std::string data)
  {
    F_LOG_UNFORMATTED("%s", data.c_str());
  });
  */
}
