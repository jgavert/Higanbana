#include "higanbana/graphics/vk/vkresources.hpp"
#include "higanbana/graphics/vk/vkdevice.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/barrier_solver.hpp"
#include "higanbana/graphics/desc/timing.hpp"
#include "higanbana/graphics/desc/resource_state.hpp"

#include <higanbana/core/profiling/profiling.hpp>
#include <higanbana/core/datastructures/proxy.hpp>

namespace higanbana
{
  namespace backend
  {
    void VulkanCommandBuffer::handleRenderpass(VulkanDevice* device, gfxpacket::RenderPassBegin& packet)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      // step1. check if renderpass is done, otherwise create renderpass
      auto& renderpassbegin = packet;

      if (!device->allResources().renderpasses[packet.renderpass].valid())
      {
        auto& rp = device->allResources().renderpasses[packet.renderpass];
        auto guard = device->deviceLock();
        int subpassIndex = 0;

        unordered_map<int64_t, int> uidToAttachmendId;
        vector<vk::SubpassDescription> subpasses;
        vector<vk::AttachmentDescription> attachments;

        // current subpass helpers
        vector<vk::AttachmentReference> colors;
        int colorsOffset = 0;
        vector<vk::AttachmentReference> depthStencil;
        vk::AttachmentReference* boundDsv = nullptr;
        bool startedSubpass = false;
        startedSubpass = true;

        auto& subpass = packet;
        // 1. find all the resources
        for (auto&& it : subpass.rtvs.convertToMemView())
        {
          auto found = uidToAttachmendId.find(it.resourceHandle().id);
          if (found != uidToAttachmendId.end())
          {
            colors.emplace_back(vk::AttachmentReference()
              .setAttachment(found->second)
              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));
          }
          else
          {
            auto attachmentId = static_cast<int>(attachments.size());
            uidToAttachmendId[it.resourceHandle().id] = attachmentId;
            colors.emplace_back(vk::AttachmentReference()
              .setAttachment(attachmentId)
              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));

            vk::AttachmentLoadOp load = vk::AttachmentLoadOp::eDontCare;

            if (it.loadOp() == LoadOp::Clear)
            {
              load = vk::AttachmentLoadOp::eClear;
            }
            else if (it.loadOp() == LoadOp::Load)
            {
              load = vk::AttachmentLoadOp::eLoad;
            }

            vk::AttachmentStoreOp store = vk::AttachmentStoreOp::eDontCare;

            if (it.storeOp() == StoreOp::Store)
            {
              store = vk::AttachmentStoreOp::eStore;
            }

            auto& view = device->allResources().texRTV[it];

            attachments.emplace_back(vk::AttachmentDescription()
              .setFormat(view.native().format)
              .setSamples(vk::SampleCountFlagBits::e1)
              .setLoadOp(load)
              .setStoreOp(store)
              .setStencilLoadOp(load)
              .setStencilStoreOp(store)
              .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
              .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal));
          }
        }

        if (subpass.dsv.id != ViewResourceHandle::InvalidViewId)
        {
          auto& it = subpass.dsv;
          auto found = uidToAttachmendId.find(it.resourceHandle().id);
          if (found != uidToAttachmendId.end())
          {
            depthStencil.emplace_back(vk::AttachmentReference()
              .setAttachment(found->second)
              .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal));
            boundDsv = &depthStencil.back();
          }
          else
          {
            auto attachmentId = static_cast<int>(attachments.size());
            uidToAttachmendId[it.resourceHandle().id] = attachmentId;
            depthStencil.emplace_back(vk::AttachmentReference()
              .setAttachment(attachmentId)
              .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal));

            vk::AttachmentLoadOp load = vk::AttachmentLoadOp::eDontCare;

            if (it.loadOp() == LoadOp::Clear)
            {
              load = vk::AttachmentLoadOp::eClear;
            }
            else if (it.loadOp() == LoadOp::Load)
            {
              load = vk::AttachmentLoadOp::eLoad;
            }

            vk::AttachmentStoreOp store = vk::AttachmentStoreOp::eDontCare;

            if (it.storeOp() == StoreOp::Store)
            {
              store = vk::AttachmentStoreOp::eStore;
            }

            auto& view = device->allResources().texDSV[it];

            attachments.emplace_back(vk::AttachmentDescription()
              .setFormat(view.native().format)
              .setSamples(vk::SampleCountFlagBits::e1)
              .setLoadOp(load)
              .setStoreOp(store)
              .setStencilLoadOp(load)
              .setStencilStoreOp(store)
              .setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
              .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal));

            boundDsv = &depthStencil.back();
          }
        }

        vk::AccessFlags everythingFlush = vk::AccessFlagBits::eUniformRead 
        | vk::AccessFlagBits::eShaderWrite
        | vk::AccessFlagBits::eShaderRead 
        | vk::AccessFlagBits::eColorAttachmentRead 
        | vk::AccessFlagBits::eColorAttachmentWrite 
        | vk::AccessFlagBits::eDepthStencilAttachmentRead 
        | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        vk::AccessFlags gfxAccess = vk::AccessFlagBits::eIndirectCommandRead
        | vk::AccessFlagBits::eIndexRead
        | vk::AccessFlagBits::eUniformRead 
        | vk::AccessFlagBits::eShaderWrite
        | vk::AccessFlagBits::eShaderRead 
        | vk::AccessFlagBits::eColorAttachmentRead 
        | vk::AccessFlagBits::eColorAttachmentWrite 
        | vk::AccessFlagBits::eDepthStencilAttachmentRead 
        | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        vk::SubpassDependency dependency[] = {
          vk::SubpassDependency()
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcAccessMask(everythingFlush)
            .setDstAccessMask(gfxAccess)
            .setSrcStageMask(vk::PipelineStageFlagBits::eAllCommands)
            .setDstStageMask(vk::PipelineStageFlagBits::eAllGraphics)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion),
          vk::SubpassDependency()
            .setSrcSubpass(0)
            .setDstSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcAccessMask(gfxAccess)
            .setDstAccessMask(everythingFlush)
            .setSrcStageMask(vk::PipelineStageFlagBits::eAllGraphics)
            .setDstStageMask(vk::PipelineStageFlagBits::eAllCommands)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion)};

          // handle subpass here with renderpass creation.
        subpasses.emplace_back(vk::SubpassDescription()
          .setColorAttachmentCount(static_cast<int>(colors.size()))
          .setPColorAttachments(colors.data())
          .setPDepthStencilAttachment(boundDsv)
          .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics));

        vk::RenderPassCreateInfo rpinfo = vk::RenderPassCreateInfo()
          .setDependencyCount(0)
          .setPDependencies(dependency)
          .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
          .setSubpassCount(static_cast<uint32_t>(subpasses.size()))
          .setPAttachments(attachments.data())
          .setPSubpasses(subpasses.data());

        auto rpobj = device->createRenderpass(rpinfo);

        rp.native() = rpobj;
        rp.setValid();
      }

      auto& rp = device->allResources().renderpasses[packet.renderpass];
      // step2. collect and register framebuffer to renderpass
      unordered_map<int64_t, int> uidToAttachmendId;
      vector<vk::ImageView> attachments;

      int fbWidth = -1;
      int fbHeight = -1;

      auto& subpass = packet;

      for (auto&& it : subpass.rtvs.convertToMemView())
      {
        auto found = uidToAttachmendId.find(it.resourceHandle().id);
        if (found == uidToAttachmendId.end())
        {
          auto& view = device->allResources().texRTV[it];
          auto& desc = device->allResources().tex[it.resourceHandle()].desc();
          if (fbWidth == fbHeight && fbHeight == -1)
          {
            fbWidth = static_cast<int>(desc.desc.width);
            fbHeight = static_cast<int>(desc.desc.height);
          }
          HIGAN_ASSERT(fbWidth == static_cast<int>(desc.desc.width) && fbHeight == static_cast<int>(desc.desc.height), "Width and height must be same.");
          auto attachmentId = static_cast<int>(attachments.size());
          uidToAttachmendId[it.resourceHandle().id] = attachmentId;
          attachments.emplace_back(view.native().view);
        }
      }
      if (subpass.dsv.id != ViewResourceHandle::InvalidViewId)
      {
        // have dsv
        auto found = uidToAttachmendId.find(subpass.dsv.resourceHandle().id);
        if (found == uidToAttachmendId.end())
        {
          auto& view = device->allResources().texDSV[subpass.dsv];
          auto& desc = device->allResources().tex[subpass.dsv.resourceHandle()].desc();
          if (fbWidth == fbHeight && fbHeight == -1)
          {
            fbWidth = static_cast<int>(desc.desc.width);
            fbHeight = static_cast<int>(desc.desc.height);
          }
          HIGAN_ASSERT(fbWidth <= static_cast<int>(desc.desc.width) && fbHeight <= static_cast<int>(desc.desc.height), "Width and height must be correct.");
          auto attachmentId = static_cast<int>(attachments.size());
          uidToAttachmendId[subpass.dsv.resourceHandle().id] = attachmentId;
          attachments.emplace_back(view.native().view);
        }
      }
      renderpassbegin.fbWidth = fbWidth;
      renderpassbegin.fbHeight = fbHeight;

      {
        vk::FramebufferCreateInfo info = vk::FramebufferCreateInfo()
          .setWidth(static_cast<uint32_t>(fbWidth))
          .setHeight(static_cast<uint32_t>(fbHeight))
          .setLayers(1)
          .setPAttachments(attachments.data()) // imageview
          .setRenderPass(rp.native())
          .setAttachmentCount(static_cast<uint32_t>(attachments.size()));

        auto fbRes = device->native().createFramebuffer(info);
        VK_CHECK_RESULT(fbRes);
        m_framebuffers.push_back(std::shared_ptr<vk::Framebuffer>(new vk::Framebuffer(fbRes.value), [dev = device->native()](vk::Framebuffer* ptr)
        {
          dev.destroyFramebuffer(*ptr);
          delete ptr;
        }));
      }
    }

    void handle(vk::CommandBuffer buffer, Resources& res, gfxpacket::RenderPassBegin& packet, vk::Framebuffer fb)
    {
      auto& rp = res.renderpasses[packet.renderpass];
      
      vk::ClearValue clearValues[8];
      uint32_t attachmentCount = 0;

      for (auto&& rtv : packet.rtvs.convertToMemView())
      {
        if (rtv.loadOp() == LoadOp::Clear)
        {
          auto v = packet.clearValues.convertToMemView()[attachmentCount];
          clearValues[attachmentCount] = clearValues[attachmentCount]
            .setColor(vk::ClearColorValue().setFloat32({v.x, v.y, v.z, v.w})); 
        }
        attachmentCount++;
      }
      if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
      {
        if (packet.dsv.loadOp() == LoadOp::Clear)
        {
          auto v = packet.clearDepth;
          clearValues[attachmentCount] = clearValues[attachmentCount]
            .setDepthStencil(vk::ClearDepthStencilValue().setDepth(v).setStencil(0));
        }
        attachmentCount++;
      }
      
      auto scissorRect = vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(packet.fbWidth, packet.fbHeight));
      vk::RenderPassBeginInfo info = vk::RenderPassBeginInfo()
        .setRenderPass(rp.native())
        .setFramebuffer(fb)
        .setRenderArea(scissorRect)
        .setClearValueCount(attachmentCount)
        .setPClearValues(clearValues);

      buffer.beginRenderPass(&info, vk::SubpassContents::eInline);      

      vk::Viewport port = vk::Viewport()
        .setWidth(packet.fbWidth)
        .setHeight(-packet.fbHeight)
        .setMaxDepth(1.f)
        .setMinDepth(0.f)
        .setX(0)
        .setY(packet.fbHeight);


      buffer.setViewport(0, {port});
      buffer.setScissor(0, {scissorRect});

      //buffer.setDepthBounds(0.f, 1.f);
    }

    void handle(vk::CommandBuffer buffer, gfxpacket::Draw& packet)
    {
      buffer.draw(packet.vertexCountPerInstance, packet.instanceCount, packet.startVertex, packet.startInstance);
    }

    void handle(vk::CommandBuffer buffer, Resources& res, gfxpacket::GraphicsPipelineBind& packet)
    {
      auto& pipe = res.pipelines[packet.pipeline];
      buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe.m_pipeline);
    }

    VkUploadBlockGPU VulkanCommandBuffer::allocateConstants(size_t size)
    {
      const size_t VulkanConstantAlignment = m_constantAlignment;
      auto block = m_constantsAllocator.allocate(size, VulkanConstantAlignment);
      if (!block)
      {
        HIGAN_ASSERT(false, "never should come here");
        auto newBlock = m_constants->allocate(256 * 256 * 4); // can save tons of cpu time. the larger less need to free these.
        HIGAN_ASSERT(newBlock, "What!");
        m_allocatedConstants.push_back(newBlock);
        m_constantsAllocator = VkUploadLinearAllocatorGPU(newBlock);
        block = m_constantsAllocator.allocate(size, VulkanConstantAlignment);
      }
      HIGAN_ASSERT(block, "What!");
      return block;
    }
    // new handle binding, recycle constant+sampler sets, only update constants, bind descriptor sets directly
    void VulkanCommandBuffer::handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBindingGraphics& packet, ResourceHandle pipeline)
    {
      auto& pipe = device->allResources().pipelines[pipeline];
      auto staticSet = pipe.m_staticSet;
      auto pipeLayout = pipe.m_pipelineLayout;
      auto pconstants = packet.constantsView();
      auto block = allocateConstants(pconstants.size());
      HIGAN_ASSERT(block.block.size <= 1024, "Constants larger than 1024bytes not supported...");
      memcpy(block.data(), pconstants.data(), pconstants.size());

      vk::PipelineBindPoint bindpoint = vk::PipelineBindPoint::eGraphics;
      auto firstSet = 0;
      auto i = 0;
      bool canSkip = true;
      for (auto&& shaderArguments : packet.resourcesView())
      {
        if (canSkip && m_boundDescriptorSets[i] == shaderArguments)
        {
          firstSet++; i++;
          continue;
        }
        canSkip = false;
        m_tempSets[i] = device->allResources().shaArgs[shaderArguments].native();
        m_boundDescriptorSets[i] = shaderArguments;
        i++;
      }
      m_tempSets[i] = pipe.m_staticSet;
      vk::ArrayProxy<const vk::DescriptorSet> sets(i+1 - firstSet, m_tempSets.data() + firstSet);
      buffer.bindDescriptorSets(bindpoint, pipeLayout, firstSet, sets, {static_cast<uint32_t>(block.offset())}, m_dispatch);
    }

    void VulkanCommandBuffer::handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBindingCompute& packet, ResourceHandle pipeline)
    {
      auto& pipe = device->allResources().pipelines[pipeline];
      auto staticSet = pipe.m_staticSet;
      auto pipeLayout = pipe.m_pipelineLayout;
      auto pconstants = packet.constantsView();
      auto block = allocateConstants(pconstants.size());
      HIGAN_ASSERT(block.block.size <= 1024, "Constants larger than 1024bytes not supported...");
      memcpy(block.data(), pconstants.data(), pconstants.size());

      vk::PipelineBindPoint bindpoint = vk::PipelineBindPoint::eCompute;
      auto firstSet = 0;
      auto i = 0;
      bool canSkip = true;
      for (auto&& shaderArguments : packet.resourcesView())
      {
        if (canSkip && m_boundDescriptorSets[i] == shaderArguments)
        {
          firstSet++; i++;
          continue;
        }
        canSkip = false;
        m_tempSets[i] = device->allResources().shaArgs[shaderArguments].native();
        m_boundDescriptorSets[i] = shaderArguments;
        i++;
      }
      m_tempSets[i] = pipe.m_staticSet;
      vk::ArrayProxy<const vk::DescriptorSet> sets(i+1 - firstSet, m_tempSets.data() + firstSet);
      buffer.bindDescriptorSets(bindpoint, pipeLayout, firstSet, sets, {static_cast<uint32_t>(block.offset())}, m_dispatch);
    }

    std::string buildStageString(int stage)
    {
      std::string allStages = "";
      bool first = true;
      auto checkStage = [&](int stage, AccessStage access)
      {
        auto what = stage & access;
        //HIGAN_LOGi( "wut %u & %u == %u => %s", stage, access, what, what == access ? "True" : "False");
        if (what == access)
        {
          if (!first)
          {
            allStages += " | ";
          }
          else
          {
            first = false;
          }
          allStages += toString(access);
        }
      };

      checkStage(stage, AccessStage::Compute);
      checkStage(stage, AccessStage::Graphics);
      checkStage(stage, AccessStage::Transfer);
      checkStage(stage, AccessStage::Index);
      checkStage(stage, AccessStage::Indirect);
      checkStage(stage, AccessStage::Rendertarget);
      checkStage(stage, AccessStage::DepthStencil);
      checkStage(stage, AccessStage::Present);
      checkStage(stage, AccessStage::Raytrace);
      checkStage(stage, AccessStage::AccelerationStructure);
      return allStages;
    }

    vk::PipelineStageFlags conjureFlags(int stage)
    {
      vk::PipelineStageFlags allStages = vk::PipelineStageFlagBits::eAllCommands;
      bool first = true;
      auto checkStage = [&](int stage, AccessStage access, vk::PipelineStageFlagBits bit)
      {
        auto what = stage & access;
        if (first && what == access)
        {
          first = false;
          allStages = bit;
        }
        else if (what == access)
        {
          allStages |= bit;
        }
      };

      checkStage(stage, AccessStage::Compute, vk::PipelineStageFlagBits::eComputeShader);
      checkStage(stage, AccessStage::Graphics, vk::PipelineStageFlagBits::eAllGraphics);
      checkStage(stage, AccessStage::Transfer, vk::PipelineStageFlagBits::eTransfer);
      checkStage(stage, AccessStage::Index, vk::PipelineStageFlagBits::eVertexShader);
      checkStage(stage, AccessStage::Indirect, vk::PipelineStageFlagBits::eDrawIndirect);
      checkStage(stage, AccessStage::Rendertarget, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      checkStage(stage, AccessStage::DepthStencil, vk::PipelineStageFlagBits::eLateFragmentTests);
      checkStage(stage, AccessStage::DepthStencil, vk::PipelineStageFlagBits::eEarlyFragmentTests);
      checkStage(stage, AccessStage::Present, vk::PipelineStageFlagBits::eBottomOfPipe);
      checkStage(stage, AccessStage::Raytrace, vk::PipelineStageFlagBits::eRayTracingShaderNV);
      checkStage(stage, AccessStage::AccelerationStructure, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV);
      return allStages;
    }

    vk::AccessFlags translateAccessMask(uint32_t stage, AccessUsage usage)
    {
      vk::AccessFlags flags;
      if (AccessUsage::Read == usage || AccessUsage::ReadWrite == usage)
      {
        if (stage & AccessStage::Compute)               flags |= vk::AccessFlagBits::eShaderRead;
        if (stage & AccessStage::Graphics)              flags |= vk::AccessFlagBits::eShaderRead;
        if (stage & AccessStage::Transfer)              flags |= vk::AccessFlagBits::eTransferRead;
        if (stage & AccessStage::Index)                 flags |= vk::AccessFlagBits::eIndexRead;
        if (stage & AccessStage::Indirect)              flags |= vk::AccessFlagBits::eIndirectCommandRead;
        if (stage & AccessStage::Rendertarget)          flags |= vk::AccessFlagBits::eColorAttachmentRead;
        if (stage & AccessStage::DepthStencil)          flags |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
        if (stage & AccessStage::Present)               flags |= vk::AccessFlagBits::eMemoryRead;
        if (stage & AccessStage::Raytrace)              flags |= vk::AccessFlagBits::eShaderRead;
        if (stage & AccessStage::AccelerationStructure) flags |= vk::AccessFlagBits::eAccelerationStructureReadNV;
      }
      if (AccessUsage::Write == usage || AccessUsage::ReadWrite == usage)
      {
        if (stage & AccessStage::Compute)               flags |= vk::AccessFlagBits::eShaderWrite;
        if (stage & AccessStage::Graphics)              flags |= vk::AccessFlagBits::eShaderWrite;
        if (stage & AccessStage::Transfer)              flags |= vk::AccessFlagBits::eTransferWrite;
        if (stage & AccessStage::Index)                 flags |= vk::AccessFlagBits::eMemoryWrite; //?
        if (stage & AccessStage::Indirect)              flags |= vk::AccessFlagBits::eMemoryWrite; //?
        if (stage & AccessStage::Rendertarget)          flags |= vk::AccessFlagBits::eColorAttachmentWrite;
        if (stage & AccessStage::DepthStencil)          flags |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        if (stage & AccessStage::Present)               flags |= vk::AccessFlagBits::eMemoryWrite; //?
        if (stage & AccessStage::Raytrace)              flags |= vk::AccessFlagBits::eShaderWrite;
        if (stage & AccessStage::AccelerationStructure) flags |= vk::AccessFlagBits::eAccelerationStructureWriteNV;
      }
      return flags;
    }

    vk::ImageLayout translateLayout(backend::TextureLayout layout)
    {
      switch (layout)
      {
        case TextureLayout::General:
          return vk::ImageLayout::eGeneral;
        case TextureLayout::Rendertarget:
          return vk::ImageLayout::eColorAttachmentOptimal;
        case TextureLayout::DepthStencil:
          return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case TextureLayout::DepthStencilReadOnly:
          return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        case TextureLayout::ShaderReadOnly:
          return vk::ImageLayout::eShaderReadOnlyOptimal;
        case TextureLayout::TransferSrc:
          return vk::ImageLayout::eTransferSrcOptimal;
        case TextureLayout::TransferDst:
          return vk::ImageLayout::eTransferDstOptimal;
        case TextureLayout::Preinitialize:
          return vk::ImageLayout::ePreinitialized;
        case TextureLayout::DepthReadOnlyStencil:
          return vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal;
        case TextureLayout::StencilReadOnlyDepth:
          return vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal;
        case TextureLayout::Present:
          return vk::ImageLayout::ePresentSrcKHR;
        case TextureLayout::SharedPresent:
          return vk::ImageLayout::eSharedPresentKHR;
        case TextureLayout::ShadingRateNV:
          return vk::ImageLayout::eShadingRateOptimalNV;
        case TextureLayout::FragmentDensityMap:
          return vk::ImageLayout::eFragmentDensityMapOptimalEXT;
        case TextureLayout::Undefined:
        default:
        break;
      }
      return vk::ImageLayout::eUndefined;
    }

    void addBarrier(VulkanDevice* device, vk::CommandBuffer buffer, MemoryBarriers barriers)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto idx = device->queueIndexes();
      if (!barriers.buffers.empty() || !barriers.textures.empty())
      {
        int beforeStage = backend::AccessStage::Common; // for pipelineStage barrier
        int afterStage = backend::AccessStage::Common; // for pipelineStage barrier
        vector<vk::BufferMemoryBarrier> bufferbar;
        vector<vk::ImageMemoryBarrier> imagebar;
        
        for (auto& buffer : barriers.buffers)
        {
          beforeStage = beforeStage | buffer.before.stage;
          afterStage = afterStage | buffer.after.stage;

          auto& vbuffer = device->allResources().buf[buffer.handle];

          auto srcQueue = idx.queue(buffer.before.queue_index);
          auto dstQueue = idx.queue(buffer.after.queue_index);
          if (srcQueue == VK_QUEUE_FAMILY_IGNORED || dstQueue == VK_QUEUE_FAMILY_IGNORED){
            //HIGAN_ASSERT(srcQueue != dstQueue, "wtf!?");
            srcQueue = VK_QUEUE_FAMILY_IGNORED;
            dstQueue = VK_QUEUE_FAMILY_IGNORED;
          }

          bufferbar.emplace_back(vk::BufferMemoryBarrier()
            .setSrcAccessMask(translateAccessMask(buffer.before.stage, buffer.before.usage))
            .setDstAccessMask(translateAccessMask(buffer.after.stage, buffer.after.usage))
            .setSrcQueueFamilyIndex(srcQueue)
            .setDstQueueFamilyIndex(dstQueue)
            .setBuffer(vbuffer.native())
            .setOffset(0)
            .setSize(VK_WHOLE_SIZE));
          //if (buffer.before.queue_index != buffer.after.queue_index)
          //  HIGAN_ILOG("vulkan", "Woah nelly there! buffer %d -> %d", idx.queue(buffer.before.queue_index), idx.queue(buffer.after.queue_index));
        }
        for (auto& image : barriers.textures)
        {
          beforeStage = beforeStage | image.before.stage;
          afterStage = afterStage | image.after.stage;

          auto& vimage = device->allResources().tex[image.handle];

          vk::ImageSubresourceRange range = vk::ImageSubresourceRange()
            .setAspectMask(vimage.aspectFlags())
            .setBaseMipLevel(image.startMip)
            .setLevelCount(image.mipSize)
            .setBaseArrayLayer(image.startArr)
            .setLayerCount(image.arrSize);

          if (image.before.queue_index != image.after.queue_index)
            HIGAN_ILOG("vulkan", "Woah nelly there! Texture %d -> %d", idx.queue(image.before.queue_index), idx.queue(image.after.queue_index));
          imagebar.emplace_back(vk::ImageMemoryBarrier()
            .setSrcAccessMask(translateAccessMask(image.before.stage, image.before.usage))
            .setDstAccessMask(translateAccessMask(image.after.stage, image.after.usage))
            .setSrcQueueFamilyIndex(idx.queue(image.before.queue_index))
            .setDstQueueFamilyIndex(idx.queue(image.after.queue_index))
            .setNewLayout(translateLayout(image.after.layout))
            .setOldLayout(translateLayout(image.before.layout))
            .setImage(vimage.native())
            .setSubresourceRange(range));
        }
        //auto beforeStr = buildStageString(beforeStage);
        //auto afterStr = buildStageString(afterStage);
        //HIGAN_ILOG("vkBarriers", "need to conjure some barriers: before:\"%s\" after:\"%s\"", beforeStr.c_str(), afterStr.c_str());
        auto vkBefore = conjureFlags(beforeStage);
        auto vkAfter = conjureFlags(afterStage);

        vk::ArrayProxy<const vk::BufferMemoryBarrier> buffers(bufferbar.size(), bufferbar.data());
        vk::ArrayProxy<const vk::ImageMemoryBarrier> images(imagebar.size(), imagebar.data());

        //HIGAN_LOGi( "PipelineBarrier");
        buffer.pipelineBarrier(vkBefore, vkAfter, {}, {}, buffers, images);
      }
    }

    void VulkanCommandBuffer::addCommands(VulkanDevice* device, vk::CommandBuffer buffer, MemView<CommandBuffer*>& buffers, BarrierSolver& solver)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      int drawIndex = 0;
      int framebuffer = 0;
      ResourceHandle boundPipeline;
      std::string currentBlock;
      bool beganLabel = false;
      bool hasReadback = false;
      auto barrierInfoIndex = 0;
      VulkanQuery timestamp;
      auto& barrierInfos = solver.barrierInfos();
      auto barrierInfosSize = barrierInfos.size();
      backend::CommandBuffer::PacketHeader* rpbegin = nullptr;
      for (auto&& list : buffers)
      {
        for (auto iter = list->begin(); (*iter)->type != PacketType::EndOfPackets; iter++)
        {
          auto* header = *iter;
          //HIGAN_ILOG("addCommandsVK", "type header: %s", gfxpacket::packetTypeToString(header->type));
          if (barrierInfoIndex < barrierInfosSize && barrierInfos[barrierInfoIndex].drawcall == drawIndex)
          {
            addBarrier(device, buffer, solver.runBarrier(barrierInfos[barrierInfoIndex]));
            barrierInfoIndex++;
          }
          switch (header->type)
          {
            //        case CommandPacket::PacketType::BufferCopy:
            //        case CommandPacket::PacketType::Dispatch:
          case PacketType::RenderBlock:
          {
            if (m_dispatch.vkCmdBeginDebugUtilsLabelEXT)
            {
              gfxpacket::RenderBlock& packet = header->data<gfxpacket::RenderBlock>();
              auto view = packet.name.convertToMemView();
              currentBlock = std::string(view.data());
              if (beganLabel)
              {
                buffer.endDebugUtilsLabelEXT(m_dispatch);
                buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, m_querypool->native(), timestamp.endIndex, m_dispatch);
                m_queries.emplace_back(timestamp);
              }
              else
              {
                beganLabel = true;
              }
              vk::DebugUtilsLabelEXT label = vk::DebugUtilsLabelEXT().setPLabelName(view.data());
              buffer.beginDebugUtilsLabelEXT(label, m_dispatch);

              timestamp = m_querypool->allocate();
              buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, m_querypool->native(), timestamp.beginIndex, m_dispatch);
              //buffer->EndQuery(m_queryheap->native(), , timestamp.beginIndex)
              //HIGAN_LOGi("renderblock: %s\n", currentBlock.c_str());
            }
            break;
          }
          case PacketType::PrepareForPresent:
          {
            break;
          }
          case PacketType::RenderpassBegin:
          {
            handleRenderpass(device, header->data<gfxpacket::RenderPassBegin>());
            handle(buffer, device->allResources(), header->data<gfxpacket::RenderPassBegin>(), *m_framebuffers[framebuffer]);
            rpbegin = header;
            framebuffer++;
            break;
          }
          case PacketType::ScissorRect:
          {
            gfxpacket::ScissorRect& packet = header->data<gfxpacket::ScissorRect>();
            auto extent = math::sub(packet.bottomright, packet.topleft);
            auto scissorRect = vk::Rect2D(vk::Offset2D(packet.topleft.x, packet.topleft.y), vk::Extent2D(extent.x, extent.y));
            buffer.setScissor(0, {scissorRect}, m_dispatch);
            break;
          }
          case PacketType::GraphicsPipelineBind:
          {
            gfxpacket::GraphicsPipelineBind& packet = header->data<gfxpacket::GraphicsPipelineBind>();
            if (boundPipeline.id != packet.pipeline.id) {
              auto oldPipe = device->updatePipeline(packet.pipeline, rpbegin->data<gfxpacket::RenderPassBegin>());
              if (oldPipe)
              {
                m_oldPipelines.push_back(oldPipe.value());
              }
              buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, device->allResources().pipelines[packet.pipeline].m_pipeline, m_dispatch);
              boundPipeline = packet.pipeline;
            }
            break;
          }
          case PacketType::ComputePipelineBind:
          {
            gfxpacket::ComputePipelineBind& packet = header->data<gfxpacket::ComputePipelineBind>();
            if (boundPipeline.id != packet.pipeline.id) {
              auto oldPipe = device->updatePipeline(packet.pipeline);
              if (oldPipe)
              {
                m_oldPipelines.push_back(oldPipe.value());
              }
              buffer.bindPipeline(vk::PipelineBindPoint::eCompute, device->allResources().pipelines[packet.pipeline].m_pipeline, m_dispatch);
              boundPipeline = packet.pipeline;
            }

            break;
          }
          case PacketType::ResourceBindingGraphics:
          {
            gfxpacket::ResourceBindingGraphics& packet = header->data<gfxpacket::ResourceBindingGraphics>();
            handleBinding(device, buffer, packet, boundPipeline);
            break;
          }
          case PacketType::ResourceBindingCompute:
          {
            gfxpacket::ResourceBindingCompute& packet = header->data<gfxpacket::ResourceBindingCompute>();
            handleBinding(device, buffer, packet, boundPipeline);
            break;
          }
          case PacketType::Draw:
          {
            auto params = header->data<gfxpacket::Draw>();
            buffer.draw(params.vertexCountPerInstance, params.instanceCount, params.startVertex, params.startInstance, m_dispatch);
            break;
          }
          case PacketType::DrawIndexed:
          {
            auto params = header->data<gfxpacket::DrawIndexed>();
            if (params.indexbuffer.type == ViewResourceType::BufferIBV && params.indexbuffer != m_boundIndexBuffer)
            {
              auto& ibv = device->allResources().bufIBV[params.indexbuffer];
              buffer.bindIndexBuffer(ibv.native().indexBuffer, ibv.native().offset, ibv.native().indexType, m_dispatch);
              m_boundIndexBuffer = params.indexbuffer;
            }
            else if (params.indexbuffer != m_boundIndexBuffer)
            {
              auto& ibv = device->allResources().dynBuf[params.indexbuffer];
              buffer.bindIndexBuffer(ibv.native().buffer, ibv.native().block.block.offset, ibv.native().index, m_dispatch);
              m_boundIndexBuffer = params.indexbuffer;
            }
            buffer.drawIndexed(params.IndexCountPerInstance, params.instanceCount, params.StartIndexLocation, params.BaseVertexLocation, params.StartInstanceLocation, m_dispatch);
            break;
          }
          case PacketType::DispatchMesh:
          {
            auto params = header->data<gfxpacket::DispatchMesh>();
            buffer.drawMeshTasksNV(params.xDim, 0, m_dispatch);
            break;
          }
          case PacketType::Dispatch:
          {
            auto params = header->data<gfxpacket::Dispatch>();
            buffer.dispatch(params.groups.x, params.groups.y, params.groups.z, m_dispatch);
            break;
          }
          case PacketType::DrawIndirect:
          {
            auto params = header->data<gfxpacket::DrawIndirect>();
            auto& srv = device->allResources().bufSRV[params.indirectBuffer];
            auto& buf = device->allResources().buf[params.indirectBuffer.resourceHandle()];
            if (params.countBuffer.resourceHandle().id != ResourceHandle::InvalidId) {
              auto& csrv = device->allResources().bufSRV[params.countBuffer];
              auto& cbuf = device->allResources().buf[params.countBuffer.resourceHandle()];
              buffer.drawIndirectCount(buf.native(), srv.native().offset, cbuf.native(), csrv.native().offset, params.maxCommands, sizeof(Indirect::DrawParams), m_dispatch);
            }
            else {
              buffer.drawIndirect(buf.native(), srv.native().offset, params.maxCommands, sizeof(Indirect::DrawParams), m_dispatch);
            }
            break;
          }
          case PacketType::DrawIndexedIndirect:
          {
            auto params = header->data<gfxpacket::DrawIndexedIndirect>();
            if (params.indexbuffer.type == ViewResourceType::BufferIBV && params.indexbuffer != m_boundIndexBuffer)
            {
              auto& ibv = device->allResources().bufIBV[params.indexbuffer];
              buffer.bindIndexBuffer(ibv.native().indexBuffer, ibv.native().offset, ibv.native().indexType, m_dispatch);
              m_boundIndexBuffer = params.indexbuffer;
            }
            else if (params.indexbuffer != m_boundIndexBuffer)
            {
              auto& ibv = device->allResources().dynBuf[params.indexbuffer];
              buffer.bindIndexBuffer(ibv.native().buffer, ibv.native().block.block.offset, ibv.native().index, m_dispatch);
              m_boundIndexBuffer = params.indexbuffer;
            }
            auto& srv = device->allResources().bufSRV[params.indirectBuffer];
            auto& buf = device->allResources().buf[params.indirectBuffer.resourceHandle()];
            if (params.countBuffer.resourceHandle().id != ResourceHandle::InvalidId) {
              auto& csrv = device->allResources().bufSRV[params.countBuffer];
              auto& cbuf = device->allResources().buf[params.countBuffer.resourceHandle()];
              buffer.drawIndexedIndirectCount(buf.native(), srv.native().offset, cbuf.native(), csrv.native().offset, params.maxCommands, sizeof(Indirect::DrawIndexedParams), m_dispatch);
            }
            else {
              buffer.drawIndexedIndirect(buf.native(), srv.native().offset, params.maxCommands, sizeof(Indirect::DrawIndexedParams), m_dispatch);
            }
            break;
          }
          case PacketType::DispatchIndirect:
          {
            auto params = header->data<gfxpacket::DispatchIndirect>();
            auto& srv = device->allResources().bufSRV[params.indirectBuffer];
            auto& buf = device->allResources().buf[params.indirectBuffer.resourceHandle()];
            buffer.dispatchIndirect(buf.native(), srv.native().offset, m_dispatch);
            break;
          }
          case PacketType::DispatchRaysIndirect:
          {
            HIGAN_ASSERT(false, "unimplemented");
            /*
            auto params = header->data<gfxpacket::DrawIndirect>();
            auto& srv = device->allResources().bufSRV[params.indirectBuffer];
            auto& buf = device->allResources().buf[params.indirectBuffer.resourceHandle()];
            if (params.countBuffer.resourceHandle().id != ResourceHandle::InvalidId) {
              auto& csrv = device->allResources().bufSRV[params.countBuffer];
              auto& cbuf = device->allResources().buf[params.countBuffer.resourceHandle()];
              buffer.drawIndirectCount(buf.native(), srv.native().offset, cbuf.native(), csrv.native().offset, params.maxCommands, sizeof(Indirect::DispatchRaysParams), m_dispatch);
            }
            else {
              buffer.traceRaysIndirectKHR(buf.native(), srv.native().offset, params.maxCommands, sizeof(Indirect::DispatchRaysParams), m_dispatch);
            }
            */
            break;
          }
          case PacketType::DispatchMeshIndirect:
          {
            auto params = header->data<gfxpacket::DispatchMeshIndirect>();
            auto& srv = device->allResources().bufSRV[params.indirectBuffer];
            auto& buf = device->allResources().buf[params.indirectBuffer.resourceHandle()];
            if (params.countBuffer.resourceHandle().id != ResourceHandle::InvalidId) {
              auto& csrv = device->allResources().bufSRV[params.countBuffer];
              auto& cbuf = device->allResources().buf[params.countBuffer.resourceHandle()];
              buffer.drawMeshTasksIndirectCountNV(buf.native(), srv.native().offset, cbuf.native(), csrv.native().offset, params.maxCommands, sizeof(Indirect::DispatchParams), m_dispatch);
            }
            else {
              buffer.drawMeshTasksIndirectNV(buf.native(), srv.native().offset, params.maxCommands, sizeof(Indirect::DispatchParams), m_dispatch);
            }
            break;
          }
          case PacketType::BufferCopy:
          {
            auto params = header->data<gfxpacket::BufferCopy>();
            auto dst = device->allResources().buf[params.dst].native();
            auto src = device->allResources().buf[params.src].native();

            vk::BufferCopy info = vk::BufferCopy()
              .setSrcOffset(params.srcOffset)
              .setDstOffset(params.dstOffset)
              .setSize(params.numBytes);

            buffer.copyBuffer(src, dst, {info});
            break;
          }
          case PacketType::UpdateTexture:
          {
            auto params = header->data<gfxpacket::UpdateTexture>();
            auto texture = device->allResources().tex[params.tex];
            auto dynamic = device->allResources().dynBuf[params.dynamic].native();

            auto rows = dynamic.block.size() / dynamic.rowPitch;

            vk::ImageSubresourceLayers layers = vk::ImageSubresourceLayers()
              .setMipLevel(params.mip)
              .setLayerCount(params.slice)
              .setLayerCount(1)
              .setAspectMask(texture.aspectFlags());

            vk::BufferImageCopy info = vk::BufferImageCopy()
              .setBufferOffset(dynamic.block.block.offset)
              .setBufferRowLength(params.srcbox.size().x)
              .setBufferImageHeight(params.srcbox.size().y)
              .setImageOffset(vk::Offset3D(params.dstPos.x, params.dstPos.y, params.dstPos.z))
              .setImageExtent(vk::Extent3D(params.srcbox.size().x, params.srcbox.size().y, params.srcbox.size().z))
              .setImageSubresource(layers);

            buffer.copyBufferToImage(dynamic.buffer, texture.native(), vk::ImageLayout::eTransferDstOptimal, {info});
            break;
          }
          case PacketType::TextureToBufferCopy:
          {
            auto params = header->data<gfxpacket::TextureToBufferCopy>();
            auto srcTex = device->allResources().tex[params.srcTexture];
            auto dstBuf = device->allResources().buf[params.dstBuffer];

            auto rows = params.height;

            vk::ImageSubresourceLayers layers = vk::ImageSubresourceLayers()
              .setMipLevel(params.mip)
              .setLayerCount(params.slice)
              .setLayerCount(1)
              .setAspectMask(srcTex.aspectFlags());

            auto rowLength = sizeFormatRowPitch(int2(params.width, params.height), params.format) / formatSizeInfo(params.format).pixelSize;

            vk::BufferImageCopy info = vk::BufferImageCopy()
              .setBufferOffset(params.dstOffset)
              .setBufferRowLength(rowLength)
              .setBufferImageHeight(params.height)
              .setImageOffset(vk::Offset3D(0, 0, 0))
              .setImageExtent(vk::Extent3D(params.width, params.height, 1))
              .setImageSubresource(layers);
            
            buffer.copyImageToBuffer(srcTex.native(), vk::ImageLayout::eTransferSrcOptimal, dstBuf.native(), {info});
            break;
          }
          case PacketType::BufferToTextureCopy:
          {
            auto params = header->data<gfxpacket::BufferToTextureCopy>();
            auto srcBuf = device->allResources().buf[params.srcBuffer];
            auto dstTex = device->allResources().tex[params.dstTexture];

            vk::ImageSubresourceLayers layers = vk::ImageSubresourceLayers()
              .setMipLevel(params.mip)
              .setLayerCount(params.slice)
              .setLayerCount(1)
              .setAspectMask(dstTex.aspectFlags());

            auto rowLength = sizeFormatRowPitch(int2(params.width, params.height), params.format) / formatSizeInfo(params.format).pixelSize;

            vk::BufferImageCopy info = vk::BufferImageCopy()
              .setBufferOffset(params.srcOffset)
              .setBufferRowLength(rowLength)
              .setBufferImageHeight(params.height)
              .setImageOffset(vk::Offset3D(0, 0, 0))
              .setImageExtent(vk::Extent3D(params.width, params.height, 1))
              .setImageSubresource(layers);
            
            buffer.copyBufferToImage(srcBuf.native(), dstTex.native(), vk::ImageLayout::eTransferDstOptimal, {info});
            break;
          }
          case PacketType::TextureToTextureCopy:
          {
            auto params = header->data<gfxpacket::TextureToTextureCopy>();
            auto dst = device->allResources().tex[params.dst];
            auto src = device->allResources().tex[params.src];

            vk::ImageCopy icopy = vk::ImageCopy()
              .setDstOffset(vk::Offset3D().setX(params.dstPos.x).setY(params.dstPos.y).setZ(params.dstPos.z))
              .setDstSubresource(vk::ImageSubresourceLayers().setMipLevel(params.dstMip).setLayerCount(1).setBaseArrayLayer(params.dstSlice).setAspectMask(vk::ImageAspectFlagBits::eColor))
              .setSrcSubresource(vk::ImageSubresourceLayers().setMipLevel(params.srcMip).setLayerCount(1).setBaseArrayLayer(params.srcSlice).setAspectMask(vk::ImageAspectFlagBits::eColor))
              .setExtent(vk::Extent3D().setWidth(params.srcbox.rightBottomBack.x).setHeight(params.srcbox.rightBottomBack.y).setDepth(params.srcbox.rightBottomBack.z));

            buffer.copyImage(src.native(), vk::ImageLayout::eTransferSrcOptimal, dst.native(), vk::ImageLayout::eTransferDstOptimal, {icopy});
            break;
          }
          case PacketType::RenderpassEnd:
          {
            buffer.endRenderPass();
            break;
          }
          case PacketType::DynamicBufferCopy:
          {
            auto params = header->data<gfxpacket::DynamicBufferCopy>();
            auto dst = device->allResources().buf[params.dst].native();
            auto& src = device->allResources().dynBuf[params.src];

            vk::BufferCopy info = vk::BufferCopy()
              .setSrcOffset(src.indexOffset())
              .setDstOffset(params.dstOffset)
              .setSize(params.numBytes);

            buffer.copyBuffer(src.native().buffer, dst, info);
            break;
          }
          case PacketType::ReadbackTexture:
          {
            auto params = header->data<gfxpacket::ReadbackTexture>();
            auto srcTex = device->allResources().tex[params.src];
            auto dstBuf = device->allResources().rbBuf[params.dst];

            auto ss = params.srcbox.size();
            auto rows = ss.y;

            vk::ImageSubresourceLayers layers = vk::ImageSubresourceLayers()
              .setMipLevel(params.mip)
              .setLayerCount(params.slice)
              .setLayerCount(1)
              .setAspectMask(srcTex.aspectFlags());

            auto rowLength = sizeFormatRowPitch(ss, params.format) / formatSizeInfo(params.format).pixelSize;

            auto start = params.srcbox.leftTopFront;

            vk::BufferImageCopy info = vk::BufferImageCopy()
              .setBufferOffset(0)
              .setBufferRowLength(rowLength)
              .setBufferImageHeight(ss.y)
              .setImageOffset(vk::Offset3D(start.x, start.y, start.z))
              .setImageExtent(vk::Extent3D(ss.x, ss.y, ss.z))
              .setImageSubresource(layers);
            
            buffer.copyImageToBuffer(srcTex.native(), vk::ImageLayout::eTransferSrcOptimal, dstBuf.native(), {info});
            break;
          }
          case PacketType::ReadbackBuffer:
          {
            auto params = header->data<gfxpacket::ReadbackBuffer>();
            auto src = device->allResources().buf[params.src];
            auto dst = device->allResources().rbBuf[params.dst];
            vk::BufferCopy region = vk::BufferCopy()
              .setSrcOffset(params.srcOffset)
              .setDstOffset(dst.offset())
              .setSize(params.numBytes);
            buffer.copyBuffer(src.native(), dst.native(), region);
            hasReadback = true;
            break;
          }
          case PacketType::ReadbackShaderDebug:
          {
            auto params = (*iter)->data<gfxpacket::ReadbackShaderDebug>();
            auto& dst = device->allResources().rbBuf[params.dst];
            vk::BufferCopy region = vk::BufferCopy()
              .setSrcOffset(0)
              .setDstOffset(0)
              .setSize(HIGANBANA_SHADER_DEBUG_WIDTH);

            auto barrier = vk::BufferMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setDstAccessMask(translateAccessMask(AccessStage::Transfer, AccessUsage::Read))
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setBuffer(m_shaderDebugBuffer)
            .setOffset(0)
            .setSize(VK_WHOLE_SIZE);
            buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {barrier}, {});
            buffer.copyBuffer(m_shaderDebugBuffer, dst.native(), region);
            barrier = barrier.setSrcAccessMask(translateAccessMask(AccessStage::Transfer, AccessUsage::Read))
                              .setDstAccessMask(translateAccessMask(AccessStage::Transfer, AccessUsage::Write));
            buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {barrier}, {});
            buffer.fillBuffer(m_shaderDebugBuffer, 0, HIGANBANA_SHADER_DEBUG_WIDTH, 0);
            hasReadback = true;
            break;
          }
          default:
            break;
          }
          drawIndex++;
        }
      }
      if (beganLabel)
      {
        buffer.endDebugUtilsLabelEXT(device->dispatcher());
        buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, m_querypool->native(), timestamp.endIndex, m_dispatch);
        m_queries.emplace_back(timestamp);

        //readback = m_readback->allocate(m_querypool->size()*m_querypool->counterSize());
        //vk::QueryResultFlags flags = vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait; 
        //buffer.copyQueryPoolResults(m_querypool->native(), 0, uint32_t(m_querypool->size()), m_readback->buffer(), readback.offset, 8, flags, m_dispatch);
      }
      if (hasReadback) {
        vk::MemoryBarrier barrier = vk::MemoryBarrier().setSrcAccessMask(vk::AccessFlagBits::eTransferWrite).setDstAccessMask(vk::AccessFlagBits::eHostRead);
        m_list->list().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, barrier, {}, {}, m_dispatch);
      }
    }

    void VulkanCommandBuffer::preprocess(VulkanDevice* device, MemView<backend::CommandBuffer*>& buffers)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      backend::CommandBuffer::PacketHeader* rpbegin = nullptr;
      // find all renderpasses & compile all missing graphics pipelines
      for (auto&& list : buffers)
      {
        for (auto iter = list->begin(); (*iter)->type != backend::PacketType::EndOfPackets; iter++)
        {
          switch ((*iter)->type)
          {
            case PacketType::RenderpassBegin:
            {
              rpbegin = (*iter);
              gfxpacket::RenderPassBegin& packet = (*iter)->data<gfxpacket::RenderPassBegin>();
              handleRenderpass(device, packet);
              break;
            }
            case PacketType::GraphicsPipelineBind:
            {
              gfxpacket::GraphicsPipelineBind& packet = (*iter)->data<gfxpacket::GraphicsPipelineBind>();
              auto oldPipe = device->updatePipeline(packet.pipeline, rpbegin->data<gfxpacket::RenderPassBegin>());
              if (oldPipe)
              {
                m_oldPipelines.push_back(oldPipe.value());
              }
              break;
            }
            case PacketType::ComputePipelineBind:
            {
              gfxpacket::ComputePipelineBind& packet = (*iter)->data<gfxpacket::ComputePipelineBind>();
              auto oldPipe = device->updatePipeline(packet.pipeline);
              if (oldPipe)
              {
                m_oldPipelines.push_back(oldPipe.value());
              }
              break;
            }
            case PacketType::RenderpassEnd:
            {
              rpbegin = nullptr;
              break;
            }
            default:
            break;
          }
        }
      }
    }

    // implementations

    void VulkanCommandBuffer::reserveConstants(size_t expectedTotalBytes) {
      auto newBlock = m_constants->allocate(expectedTotalBytes); // can save tons of cpu time. the larger less need to free these.
      HIGAN_ASSERT(newBlock, "What!");
      m_allocatedConstants.push_back(newBlock);
      m_constantsAllocator = VkUploadLinearAllocatorGPU(newBlock);
    }

    void VulkanCommandBuffer::beginConstantsDmaList(std::shared_ptr<prototypes::DeviceImpl> device) {
      HIGAN_CPU_BRACKET("reset?");
      auto nat = std::static_pointer_cast<VulkanDevice>(device);
      nat->native().resetQueryPool(m_querypool->native(), 0, uint32_t(m_querypool->max_size()), m_dispatch);
      VK_CHECK_RESULT_RAW(m_list->list().begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(nullptr)));

      auto timestamp = m_querypool->allocate();
      m_queries.push_back(timestamp);
      m_list->list().writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, m_querypool->native(), timestamp.beginIndex, m_dispatch);
    }
    void VulkanCommandBuffer::addConstants(CommandBufferImpl* buffer) {
      VulkanCommandBuffer* other = static_cast<VulkanCommandBuffer*>(buffer);
      for (auto& constants : other->m_allocatedConstants)
      {
        vk::BufferCopy copyInfo = vk::BufferCopy()
          .setSrcOffset(constants.block.offset)
          .setSize(constants.block.size)
          .setDstOffset(constants.block.offset);
        m_list->list().copyBuffer(constants.bufferCPU(), constants.bufferGPU(), copyInfo);
      }
    }
//#define CHECK_DMA_COPY
    void VulkanCommandBuffer::endConstantsDmaList() {
      vk::MemoryBarrier barrier = vk::MemoryBarrier().setSrcAccessMask(vk::AccessFlagBits::eTransferWrite).setDstAccessMask(vk::AccessFlagBits::eUniformRead);
      m_list->list().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, barrier, {}, {});
      m_list->list().writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, m_querypool->native(), m_queries.back().endIndex, m_dispatch);
#ifdef CHECK_DMA_COPY
      readback = m_readback->allocate(m_querypool->size()*m_querypool->counterSize());
      vk::QueryResultFlags flags = vk::QueryResultFlagBits::e64;// | vk::QueryResultFlagBits::eWait; 
      m_list->list().copyQueryPoolResults(m_querypool->native(), 0, uint32_t(m_querypool->size()), m_readback->buffer(), readback.offset, 8, flags);
#else
      //m_queries.clear();
#endif
      VK_CHECK_RESULT_RAW(m_list->list().end());
    }

    void VulkanCommandBuffer::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, MemView<backend::CommandBuffer*>& buffers, BarrierSolver& solver)
    {
      HIGAN_CPU_BRACKET("compile to Vulkan CmdList");
      m_tempSets.resize(5);
      auto nat = std::static_pointer_cast<VulkanDevice>(device);
      nat->native().resetQueryPool(m_querypool->native(), 0, uint32_t(m_querypool->max_size()), m_dispatch);
      
      {
        HIGAN_CPU_BRACKET("reset?");
        VK_CHECK_RESULT_RAW(m_list->list().begin(vk::CommandBufferBeginInfo()
          .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
          .setPInheritanceInfo(nullptr)));
      }

      {
        HIGAN_CPU_BRACKET("compile");
        // add commands to list while also adding barriers
        addCommands(nat.get(), m_list->list(), buffers, solver);
      }

      VK_CHECK_RESULT_RAW(m_list->list().end());
    }

    bool VulkanCommandBuffer::readbackTimestamps(std::shared_ptr<prototypes::DeviceImpl> device, vector<GraphNodeTiming>& nodes)
    {
      if (m_queries.empty())
        return false;
      HIGAN_CPU_FUNCTION_SCOPE();
      VulkanDevice* dev = static_cast<VulkanDevice*>(device.get());
      //m_readback->map(dev->native());
      //auto view = m_readback->getView(readback);
      auto ticksPerSecond = m_querypool->getGpuTicksPerSecond();
      //auto properView = reinterpret_memView<uint64_t>(view);
      auto queueTimeOffset = 0ull;

      vk::QueryResultFlags flags = vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait; 
      auto res = dev->native().getQueryPoolResults<uint64_t>(m_querypool->native(), 0, uint32_t(m_querypool->size()), uint32_t(m_querypool->size())*sizeof(uint64_t), sizeof(uint64_t), flags, m_dispatch);
      VK_CHECK_RESULT(res);
      auto properView = res.value;

      /*
      auto queueTimeOffset = dev->m_graphicsTimeOffset;
      if (m_type == QueueType::Dma)
        queueTimeOffset = dev->m_dmaTimeOffset;
      else if (m_type == QueueType::Compute)
        queueTimeOffset = dev->m_computeTimeOffset;
        */

      if (nodes.size() >= m_queries.size())
      {
        for (int i = 0; i < m_queries.size(); ++i)
        {
          auto query = m_queries[i];
          auto countParts = [ticksPerSecond](uint64_t timestamp){
            return uint64_t((double(timestamp)/double(ticksPerSecond))*1000000000ull);
          };
          uint64_t begin = countParts(properView[query.beginIndex]);
          uint64_t end = countParts(properView[query.endIndex]);
          auto& node = nodes[i];
          node.gpuTime.begin = begin + queueTimeOffset;
          node.gpuTime.end = end + queueTimeOffset;
        }
      }
      //m_readback->unmap(dev->native());
      m_readback->reset();
      //m_queryheap->reset();
      return true;
    }
  }
}