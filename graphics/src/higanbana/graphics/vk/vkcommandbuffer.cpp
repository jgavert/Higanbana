#include "higanbana/graphics/vk/vkresources.hpp"
#include "higanbana/graphics/vk/vkdevice.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/barrier_solver.hpp"
#include "higanbana/graphics/desc/resource_state.hpp"

#include <higanbana/core/datastructures/proxy.hpp>

namespace higanbana
{
  namespace backend
  {
    void VulkanCommandBuffer::handleRenderpass(VulkanDevice* device, gfxpacket::RenderPassBegin& packet)
    {
      // step1. check if renderpass is done, otherwise create renderpass
      auto& renderpassbegin = packet;
      auto& rp = device->allResources().renderpasses[packet.renderpass];

      if (!rp.valid())
      {
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
          auto* found = uidToAttachmendId.find(it.resource.id);
          if (found != uidToAttachmendId.end())
          {
            colors.emplace_back(vk::AttachmentReference()
              .setAttachment(found->second)
              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));
          }
          else
          {
            auto attachmentId = static_cast<int>(attachments.size());
            uidToAttachmendId[it.resource.id] = attachmentId;
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
          auto* found = uidToAttachmendId.find(it.resource.id);
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
            uidToAttachmendId[it.resource.id] = attachmentId;
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

      // step2. collect and register framebuffer to renderpass
      unordered_map<int64_t, int> uidToAttachmendId;
      vector<vk::ImageView> attachments;

      int fbWidth = -1;
      int fbHeight = -1;

      auto& subpass = packet;

      for (auto&& it : subpass.rtvs.convertToMemView())
      {
        auto* found = uidToAttachmendId.find(it.resource.id);
        if (found == uidToAttachmendId.end())
        {
          auto& view = device->allResources().texRTV[it];
          auto& desc = device->allResources().tex[it.resource].desc();
          if (fbWidth == fbHeight && fbHeight == -1)
          {
            fbWidth = static_cast<int>(desc.desc.width);
            fbHeight = static_cast<int>(desc.desc.height);
          }
          HIGAN_ASSERT(fbWidth == static_cast<int>(desc.desc.width) && fbHeight == static_cast<int>(desc.desc.height), "Width and height must be same.");
          auto attachmentId = static_cast<int>(attachments.size());
          uidToAttachmendId[it.resource.id] = attachmentId;
          attachments.emplace_back(view.native().view);
        }
      }
      if (subpass.dsv.id != ViewResourceHandle::InvalidViewId)
      {
        // have dsv
        auto* found = uidToAttachmendId.find(subpass.dsv.resource.id);
        if (found == uidToAttachmendId.end())
        {
          auto& view = device->allResources().texDSV[subpass.dsv];
          auto& desc = device->allResources().tex[subpass.dsv.resource].desc();
          if (fbWidth == fbHeight && fbHeight == -1)
          {
            fbWidth = static_cast<int>(desc.desc.width);
            fbHeight = static_cast<int>(desc.desc.height);
          }
          HIGAN_ASSERT(fbWidth == static_cast<int>(desc.desc.width) && fbHeight == static_cast<int>(desc.desc.height), "Width and height must be correct.");
          auto attachmentId = static_cast<int>(attachments.size());
          uidToAttachmendId[subpass.dsv.resource.id] = attachmentId;
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

    VkUploadBlock VulkanCommandBuffer::allocateConstants(size_t size)
    {
      const size_t VulkanConstantAlignment = m_constantAlignment;
      auto block = m_constantsAllocator.allocate(size, VulkanConstantAlignment);
      if (!block)
      {
        auto newBlock = m_constants->allocate(VulkanConstantAlignment * 32);
        HIGAN_ASSERT(newBlock, "What!");
        m_allocatedConstants.push_back(newBlock);
        m_constantsAllocator = VkUploadLinearAllocator(newBlock);
        block = m_constantsAllocator.allocate(size, VulkanConstantAlignment);
      }
      HIGAN_ASSERT(block, "What!");
      return block;
    }
    // new handle binding, recycle constant+sampler sets, only update constants, bind descriptor sets directly

    void VulkanCommandBuffer::handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBinding& packet, ResourceHandle pipeline)
    {
      // get dynamicbuffer for constants
      // collect otherviews and indexes
      // build... DescriptorSet... I need the pipeline
      auto defaultDescLayout = device->defaultDescLayout();
      auto layout = device->allResources().pipelines[pipeline].m_pipelineLayout;
      auto set = device->allResources().pipelines[pipeline].m_staticSet;

      auto pconstants = packet.constants.convertToMemView();
      auto block = allocateConstants(pconstants.size());
      HIGAN_ASSERT(block.block.size <= 1024, "Constants larger than 1024bytes not supported...");
      memcpy(block.data(), pconstants.data(), pconstants.size());

      vk::PipelineBindPoint bindpoint = vk::PipelineBindPoint::eGraphics;
      if (packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Compute)
      {
        bindpoint = vk::PipelineBindPoint::eCompute;
      }
      else if (packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Raytracing)
      {
        bindpoint = vk::PipelineBindPoint::eRayTracingNV;
      }
      m_tempSets.clear();
      auto firstSet = 0;
      auto i = 0;
      bool canSkip = true;
      for (auto&& shaderArguments : packet.resources.convertToMemView())
      {
        if (canSkip && m_boundDescriptorSets[i] == shaderArguments)
        {
          firstSet++; i++;
          continue;
        }
        canSkip = false;
        m_tempSets.push_back(device->allResources().shaArgs[shaderArguments].native());
        m_boundDescriptorSets[i] = shaderArguments;
        i++;
      }
      m_tempSets.push_back(set);
      buffer.bindDescriptorSets(bindpoint, layout, firstSet, m_tempSets, {static_cast<uint32_t>(block.offset())});
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

    vk::AccessFlags translateAccessMask(AccessStage stage, AccessUsage usage)
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

          bufferbar.emplace_back(vk::BufferMemoryBarrier()
            .setSrcAccessMask(translateAccessMask(buffer.before.stage, buffer.before.usage))
            .setDstAccessMask(translateAccessMask(buffer.after.stage, buffer.after.usage))
            .setSrcQueueFamilyIndex(idx.queue(buffer.before.queue_index))
            .setDstQueueFamilyIndex(idx.queue(buffer.after.queue_index))
            .setBuffer(vbuffer.native())
            .setOffset(0)
            .setSize(VK_WHOLE_SIZE));
          if (buffer.after.queue_index != buffer.after.queue_index)
            HIGAN_ILOG("vulkan", "Woah nelly there! buffer %d -> %d", idx.queue(buffer.before.queue_index), idx.queue(buffer.after.queue_index));
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

          if (image.after.queue_index != image.after.queue_index)
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

    void VulkanCommandBuffer::addCommands(VulkanDevice* device, vk::CommandBuffer buffer, MemView<CommandBuffer>& buffers, BarrierSolver& solver)
    {
      int drawIndex = 0;
      int framebuffer = 0;
      ResourceHandle boundPipeline;
      std::string currentBlock;
      bool beganLabel = false;
      auto barrierInfoIndex = 0;
      auto& barrierInfos = solver.barrierInfos();
      auto barrierInfosSize = barrierInfos.size();
      for (auto&& list : buffers)
      {
        for (auto iter = list.begin(); (*iter)->type != PacketType::EndOfPackets; iter++)
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
            if (device->dispatcher().vkCmdBeginDebugUtilsLabelEXT)
            {
              gfxpacket::RenderBlock& packet = header->data<gfxpacket::RenderBlock>();
              auto view = packet.name.convertToMemView();
              currentBlock = std::string(view.data());
              if (beganLabel)
              {
                buffer.endDebugUtilsLabelEXT(device->dispatcher());
              }
              else
              {
                beganLabel = true;
              }
              vk::DebugUtilsLabelEXT label = vk::DebugUtilsLabelEXT().setPLabelName(view.data());
              buffer.beginDebugUtilsLabelEXT(label, device->dispatcher());
            }
            break;
          }
          case PacketType::PrepareForPresent:
          {
            break;
          }
          case PacketType::RenderpassBegin:
          {
            handle(buffer, device->allResources(), header->data<gfxpacket::RenderPassBegin>(), *m_framebuffers[framebuffer]);
            framebuffer++;
            break;
          }
          case PacketType::ScissorRect:
          {
            gfxpacket::ScissorRect& packet = header->data<gfxpacket::ScissorRect>();
            auto extent = math::sub(packet.bottomright, packet.topleft);
            auto scissorRect = vk::Rect2D(vk::Offset2D(packet.topleft.x, packet.topleft.y), vk::Extent2D(extent.x, extent.y));
            buffer.setScissor(0, {scissorRect});
            break;
          }
          case PacketType::GraphicsPipelineBind:
          {
            gfxpacket::GraphicsPipelineBind& packet = header->data<gfxpacket::GraphicsPipelineBind>();
            if (boundPipeline.id != packet.pipeline.id) {
              buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, device->allResources().pipelines[packet.pipeline].m_pipeline);
              boundPipeline = packet.pipeline;
            }
            break;
          }
          case PacketType::ComputePipelineBind:
          {
            gfxpacket::ComputePipelineBind& packet = header->data<gfxpacket::ComputePipelineBind>();
            if (boundPipeline.id != packet.pipeline.id) {
              buffer.bindPipeline(vk::PipelineBindPoint::eCompute, device->allResources().pipelines[packet.pipeline].m_pipeline);
              boundPipeline = packet.pipeline;
            }

            break;
          }
          case PacketType::ResourceBinding:
          {
            gfxpacket::ResourceBinding& packet = header->data<gfxpacket::ResourceBinding>();
            handleBinding(device, buffer, packet, boundPipeline);
            break;
          }
          case PacketType::Draw:
          {
            auto params = header->data<gfxpacket::Draw>();
            buffer.draw(params.vertexCountPerInstance, params.instanceCount, params.startVertex, params.startInstance);
            break;
          }
          case PacketType::DrawIndexed:
          {
            auto params = header->data<gfxpacket::DrawIndexed>();
            if (params.indexbuffer.type == ViewResourceType::BufferIBV)
            {
              auto& ibv = device->allResources().bufIBV[params.indexbuffer];
              buffer.bindIndexBuffer(ibv.native().indexBuffer, ibv.native().offset, ibv.native().indexType);
            }
            else
            {
              auto& ibv = device->allResources().dynBuf[params.indexbuffer];
              buffer.bindIndexBuffer(ibv.native().buffer, ibv.native().block.block.offset, ibv.native().index);
            }
            buffer.drawIndexed(params.IndexCountPerInstance, params.instanceCount, params.StartIndexLocation, params.BaseVertexLocation, params.StartInstanceLocation);
            break;
          }
          case PacketType::Dispatch:
          {
            auto params = header->data<gfxpacket::Dispatch>();
            buffer.dispatch(params.groups.x, params.groups.y, params.groups.z);
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
              .setBufferRowLength(dynamic.rowPitch)
              .setBufferImageHeight(params.height)
              .setImageOffset(vk::Offset3D(0, 0, 0))
              .setImageExtent(vk::Extent3D(params.width, params.height, 1))
              .setImageSubresource(layers);

            buffer.copyBufferToImage(dynamic.buffer, texture.native(), vk::ImageLayout::eTransferDstOptimal, {info});
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
      }
    }

    void VulkanCommandBuffer::preprocess(VulkanDevice* device, MemView<backend::CommandBuffer>& buffers)
    {
      backend::CommandBuffer::PacketHeader* rpbegin = nullptr;
      // find all renderpasses & compile all missing graphics pipelines
      for (auto&& list : buffers)
      {
        for (auto iter = list.begin(); (*iter)->type != backend::PacketType::EndOfPackets; iter++)
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
    void VulkanCommandBuffer::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, MemView<backend::CommandBuffer>& buffers, BarrierSolver& solver)
    {
      auto nat = std::static_pointer_cast<VulkanDevice>(device);

      // preprocess to compile renderpasses/pipelines
      preprocess(nat.get(), buffers);
      
      m_list->list().begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(nullptr));

      // add commands to list while also adding barriers
      addCommands(nat.get(), m_list->list(), buffers, solver);

      m_list->list().end();
    }

    void VulkanCommandBuffer::readbackTimestamps(std::shared_ptr<prototypes::DeviceImpl>, vector<GraphNodeTiming>&)
    {

    }
  }
}