#include "graphics/vk/vkresources.hpp"
#include "graphics/vk/vkdevice.hpp"
#include "core/datastructures/proxy.hpp"
#include <graphics/common/command_buffer.hpp>
#include <graphics/common/barrier_solver.hpp>
#include <graphics/desc/resource_state.hpp>
namespace faze
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

        vk::AccessFlags everythingFlush = vk::AccessFlagBits::eIndirectCommandRead
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
            .setDstAccessMask(everythingFlush)
            .setSrcStageMask(vk::PipelineStageFlagBits::eAllCommands)
            .setDstStageMask(vk::PipelineStageFlagBits::eAllGraphics)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion),
          vk::SubpassDependency()
            .setSrcSubpass(0)
            .setDstSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcAccessMask(everythingFlush)
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
          .setDependencyCount(2)
          .setPDependencies(dependency)
          .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
          .setSubpassCount(static_cast<uint32_t>(subpasses.size()))
          .setPAttachments(attachments.data())
          .setPSubpasses(subpasses.data());

        auto rpobj = device->createRenderpass(rpinfo);

        rp.native() = rpobj;
        rp.setValid();
      }
      // step2. compile used pipelines against the renderpass within begin/end packets
      /*
      {
        for (CommandPacket* current = begin; current != end; current = current->nextPacket())
        {
          if (current->type() == CommandPacket::PacketType::GraphicsPipelineBind)
          {
            auto& ref = packetRef(gfxpacket::GraphicsPipelineBind, current);
            device->updatePipeline(ref.pipeline, *rp->native().get(), packetRef(gfxpacket::RenderpassBegin, begin));
            break;
          }
        }
      }
      */

      // step3. collect and register framebuffer to renderpass
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
          F_ASSERT(fbWidth == static_cast<int>(desc.desc.width) && fbHeight == static_cast<int>(desc.desc.height), "Width and height must be same.");
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
          F_ASSERT(fbWidth == static_cast<int>(desc.desc.width) && fbHeight == static_cast<int>(desc.desc.height), "Width and height must be same.");
          auto attachmentId = static_cast<int>(attachments.size());
          uidToAttachmendId[subpass.dsv.resource.id] = attachmentId;
          attachments.emplace_back(view.native().view);
        }
      }
      renderpassbegin.fbWidth = fbWidth;
      renderpassbegin.fbHeight = fbHeight;
/*
      size_t attachmentsHash = HashMemory(attachments.data(), attachments.size());
      attachmentsHash = hash_combine(hash_combine(HashMemory(&fbWidth, sizeof(fbWidth)),HashMemory(&fbHeight, sizeof(fbHeight))), attachmentsHash);
      auto& fbs = rp->framebuffers();
      auto* exists = fbs.find(attachmentsHash);
      if (exists == fbs.end())
      */
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
        //F_LOG("Created a framebuffer\n");
      }
      //rp->setActiveFramebuffer(attachmentsHash); // remember to set the current hash as active one.
    }

    void handle(vk::CommandBuffer buffer, Resources& res, gfxpacket::RenderPassBegin& packet, vk::Framebuffer fb)
    {
      auto& rp = res.renderpasses[packet.renderpass];
      
      vk::ClearValue clearValues[8];
      uint32_t attachmentCount = 0;

      for (auto&& rtv : packet.rtvs.convertToMemView())
      {
        //auto v = rtv.clearVal();
        float4 v = float4(0.f, 0.f, 0.f, 0.f);
        clearValues[attachmentCount] = clearValues[attachmentCount]
          .setColor(vk::ClearColorValue().setFloat32({v.x, v.y, v.z, v.w})); 
        attachmentCount++;        
      }
      if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
      {
        //auto v = dsv.clearVal();
        float4 v = float4(0.f, 0.f, 0.f, 0.f);
        clearValues[attachmentCount] = clearValues[attachmentCount]
          .setDepthStencil(vk::ClearDepthStencilValue().setDepth(v.x).setStencil(0));
        attachmentCount++;        
      }
      
      auto scissorRect = vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(packet.fbWidth, packet.fbHeight));
      vk::RenderPassBeginInfo info = vk::RenderPassBeginInfo()
        .setRenderPass(rp.native())
        .setFramebuffer(fb)
        .setRenderArea(scissorRect)
        .setClearValueCount(attachmentCount)
        .setPClearValues(clearValues);

      /*
      vk::AccessFlags accessFlags = vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eUniformRead | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      vk::MemoryBarrier memoryBarr = vk::MemoryBarrier().setSrcAccessMask(accessFlags).setDstAccessMask(accessFlags);
      vk::ArrayProxy<const vk::MemoryBarrier> memory(1, &memoryBarr);
      buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(), memory, {}, {});
      */

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

    void VulkanCommandBuffer::handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBinding& packet, ResourceHandle pipeline)
    {
      // get dynamicbuffer for constants
      // collect otherviews and indexes
      // build... DescriptorSet... I need the pipeline
      auto desclayout = device->allResources().pipelines[pipeline].m_descriptorSetLayout;
      auto layout = device->allResources().pipelines[pipeline].m_pipelineLayout;
      auto set = m_descriptors.allocate(device->native(), desclayout, 1)[0];
      m_allocatedSets.push_back(set);

      vector<vk::WriteDescriptorSet> writeDescriptors;

      auto constantBuffer = device->allocateConstants(packet.constants.convertToMemView());
      m_constants.push_back(constantBuffer);
      int index = 0;
      writeDescriptors.emplace_back(vk::WriteDescriptorSet()
        .setDstSet(set)
        .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
        .setDescriptorCount(1)
        .setPBufferInfo(&constantBuffer.native().bufferInfo)
        .setDstBinding(index++));

      for (auto&& descriptor : packet.resources.convertToMemView())
      {
        vk::WriteDescriptorSet writeSet = vk::WriteDescriptorSet()
        .setDstSet(set)
        .setDescriptorCount(1)
        .setDstBinding(index++);
        switch (descriptor.type)
        {
          case ViewResourceType::BufferSRV:
          {
            auto& desc = device->allResources().bufSRV[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.type);
            if (desc.type == vk::DescriptorType::eUniformTexelBuffer)
              writeSet = writeSet.setPTexelBufferView(&desc.view);
            else
              writeSet = writeSet.setPBufferInfo(&desc.bufferInfo);
            break;
          }
          case ViewResourceType::BufferUAV:
          {
            auto& desc = device->allResources().bufUAV[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.type);
            if (desc.type == vk::DescriptorType::eStorageTexelBuffer)
              writeSet = writeSet.setPTexelBufferView(&desc.view);
            else
              writeSet = writeSet.setPBufferInfo(&desc.bufferInfo);
            break;
          }
          case ViewResourceType::TextureSRV:
          {
            auto& desc = device->allResources().texSRV[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.viewType)
                        .setPImageInfo(&desc.info);
            break;
          }
          case ViewResourceType::TextureUAV:
          {
            auto& desc = device->allResources().texUAV[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.viewType)
                        .setPImageInfo(&desc.info);
            break;
          }
          case ViewResourceType::DynamicBufferSRV:
          {
            auto& desc = device->allResources().dynBuf[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.type);
            if (desc.type == vk::DescriptorType::eUniformTexelBuffer)
              writeSet = writeSet.setPTexelBufferView(&desc.texelView);
            else
              writeSet = writeSet.setPBufferInfo(&desc.bufferInfo);
            break;
          }
          default:
            break;
        }

        writeDescriptors.emplace_back(writeSet);
      }

      vk::ArrayProxy<const vk::WriteDescriptorSet> writes(writeDescriptors.size(), writeDescriptors.data());
      device->native().updateDescriptorSets(writes, {});

      vk::PipelineBindPoint bindpoint = vk::PipelineBindPoint::eGraphics;
      if (packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Compute)
      {
        bindpoint = vk::PipelineBindPoint::eCompute;
      }
      if (packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Raytracing)
      {
        bindpoint = vk::PipelineBindPoint::eRayTracingNV;
      }
      //vk::ArrayProxy<const vk::DescriptorSet> sets(1, &set);
      buffer.bindDescriptorSets(bindpoint, layout, 0, {set}, {static_cast<uint32_t>(constantBuffer.native().block.block.offset)});

    }
/*
    void VulkanCommandBuffer::preprocess(VulkanDevice* device, backend::IntermediateList& list)
    {
      // find all renderpasses

      CommandPacket* beginPass = nullptr;
      CommandPacket* endPass = nullptr;

      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
        case CommandPacket::PacketType::RenderpassBegin:
        {
          F_ASSERT(beginPass == nullptr, "!?");
          beginPass = packet;
          break;
        }
        case CommandPacket::PacketType::RenderpassEnd:
        {
          F_ASSERT(beginPass != nullptr, "!?");
          endPass = packet;
          handleRenderpass(device, list, beginPass, endPass);
          beginPass = nullptr;
          endPass = nullptr;
          break;
        }
        case CommandPacket::PacketType::ComputePipelineBind:
        {
          auto& ref = packetRef(gfxpacket::ComputePipelineBind, packet);
          device->updatePipeline(ref.pipeline);
          // TODO: bind vulkan resources
          // TODO: 
          break;
        }
        default:
          break;
        }
      }
    }

    void handle(vk::CommandBuffer buffer, gfxpacket::RenderpassBegin& packet)
    {
      auto rp = std::static_pointer_cast<VulkanRenderpass>(packet.renderpass.impl());
      
      vk::ClearValue clearValues[8];
      uint32_t attachmentCount = 0;

      for (auto&& rtv : packet.rtvs)
      {
        auto v = rtv.clearVal();
        clearValues[attachmentCount] = clearValues[attachmentCount]
          .setColor(vk::ClearColorValue().setFloat32({v.x, v.y, v.z, v.w})); 
        attachmentCount++;        
      }
      for (auto&& dsv : packet.dsvs)
      {
        auto v = dsv.clearVal();
        clearValues[attachmentCount] = clearValues[attachmentCount]
          .setDepthStencil(vk::ClearDepthStencilValue().setDepth(v.x).setStencil(0));
        attachmentCount++;        
      }

      
      auto scissorRect = vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(packet.fbWidth, packet.fbHeight));
      vk::RenderPassBeginInfo info = vk::RenderPassBeginInfo()
        .setRenderPass(*rp->native())
        .setFramebuffer(rp->getActiveFramebuffer())
        .setRenderArea(scissorRect)
        .setClearValueCount(attachmentCount)
        .setPClearValues(clearValues);

      /*
      vk::AccessFlags accessFlags = vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eUniformRead | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      vk::MemoryBarrier memoryBarr = vk::MemoryBarrier().setSrcAccessMask(accessFlags).setDstAccessMask(accessFlags);
      vk::ArrayProxy<const vk::MemoryBarrier> memory(1, &memoryBarr);
      buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(), memory, {}, {});
      */
     /*

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
    }
    void handle(vk::CommandBuffer buffer, gfxpacket::RenderpassEnd&)
    {
      buffer.endRenderPass();
    }

    void handle(vk::CommandBuffer buffer, gfxpacket::Draw& packet)
    {
      buffer.draw(packet.vertexCountPerInstance, packet.instanceCount, packet.startVertex, packet.startInstance);
    }

    void handle(vk::CommandBuffer buffer, gfxpacket::GraphicsPipelineBind& packet)
    {
      auto pipeline = packet.pipeline.m_pipelines->front();
      auto* natptr = static_cast<VulkanPipeline*>(pipeline.pipeline.get());
      buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, natptr->m_pipeline);
    }

    void handle(vk::CommandBuffer buffer, gfxpacket::ClearRT& packet)
    {
      auto view = std::static_pointer_cast<VulkanTextureView>(packet.rtv.native());
      auto texture = std::static_pointer_cast<VulkanTexture>(packet.rtv.texture().native());

      // TODO: texture->state()->flags[0].layout, figure out the index.

      buffer.clearColorImage(
        texture->native(),
        vk::ImageLayout::eTransferDstOptimal, // this probably should be somehow in the command, as we hold only old information in "texture"
        vk::ClearColorValue().setFloat32({ packet.color.x, packet.color.y, packet.color.z, packet.color.w }),
        view->range());
    }

    void addCommands(vk::CommandBuffer buffer, VulkanDependencySolver& solver, backend::IntermediateList& list)
    {
      int drawIndex = 0;
      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
          //        case CommandPacket::PacketType::BufferCopy:
          //        case CommandPacket::PacketType::Dispatch:
        case CommandPacket::PacketType::ClearRT:
        {
          solver.runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::ClearRT, packet));
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          solver.runBarrier(buffer, drawIndex);
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::ResourceBinding:
        {
          break;
        }
        case CommandPacket::PacketType::RenderpassBegin:
        {
          solver.runBarrier(buffer, drawIndex);
          drawIndex++;
          handle(buffer, packetRef(gfxpacket::RenderpassBegin, packet));
          break;
        }
        case CommandPacket::PacketType::RenderpassEnd:
        {
          buffer.endRenderPass();
          break;
        }
        case CommandPacket::PacketType::GraphicsPipelineBind:
        {
          handle(buffer, packetRef(gfxpacket::GraphicsPipelineBind, packet));
          break;
        }
        case CommandPacket::PacketType::Draw:
        {
          handle(buffer, packetRef(gfxpacket::Draw, packet));
          break;
        }
        default:
          break;
        }
      }
    }

    void addDepedencyDataAndSolve(VulkanDependencySolver& solver, backend::IntermediateList& list)
    {
      int drawIndex = 0;

      auto addTextureView = [&](int index, TextureView& texView, vk::ImageLayout layout, vk::AccessFlags flags)
      {
        auto view = std::static_pointer_cast<VulkanTextureView>(texView.native());
        auto texture = std::static_pointer_cast<VulkanTexture>(texView.texture().native());
        solver.addTexture(index, texView.texture().id(), *texture, *view, static_cast<int16_t>(texView.texture().desc().desc.miplevels), layout, flags);
      };

      auto addTexture = [&](int index, Texture& texture, vk::ImageLayout layout, vk::AccessFlags flags)
      {
        auto tex = std::static_pointer_cast<VulkanTexture>(texture.native());
        SubresourceRange range{};
        range.mipOffset = 0;
        range.mipLevels = 1;
        range.sliceOffset = 0;
        range.arraySize = 1;
        solver.addTexture(index, texture.id(), *tex, static_cast<int16_t>(texture.desc().desc.miplevels), vk::ImageAspectFlagBits::eColor, layout, flags, range);
      };

      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
          //        case CommandPacket::PacketType::BufferCopy:
          //        case CommandPacket::PacketType::Dispatch:
          /*
        case CommandPacket::PacketType::BufferCopy:
        {
          auto& p = packetRef(gfxpacket::BufferCopy, packet);
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eAllCommands);
          solver.addBuffer(drawIndex, p.source.,  )
          drawIndex++;
          break;
        }
        */
       /*
        case CommandPacket::PacketType::ClearRT:
        {
          auto& p = packetRef(gfxpacket::ClearRT, packet);
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eAllCommands);
          addTextureView(drawIndex, p.rtv, vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite);
          break;
        }
        case CommandPacket::PacketType::RenderpassBegin:
        {
          auto& p = packetRef(gfxpacket::RenderpassBegin, packet);
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eAllCommands);
          for (auto&& tex : p.rtvs)
          {
            addTexture(drawIndex, tex.texture(), vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eColorAttachmentWrite);
          }
          break;
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          auto& p = packetRef(gfxpacket::PrepareForPresent, packet);
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eAllCommands);
          addTexture(drawIndex, p.texture, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eMemoryRead);
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::ResourceBinding:
        {
          break;
        }
        default:
          break;
        }
      }
      solver.makeAllBarriers();
    }
*/
    std::string buildStageString(int stage)
    {
      std::string allStages = "";
      bool first = true;
      auto checkStage = [&](int stage, AccessStage access)
      {
        auto what = stage & access;
        //F_ILOG("", "wut %u & %u == %u => %s", stage, access, what, what == access ? "True" : "False");
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
      vk::PipelineStageFlags allStages = vk::PipelineStageFlagBits::eBottomOfPipe;
      bool first = true;
      auto checkStage = [&](int stage, AccessStage access, vk::PipelineStageFlagBits bit)
      {
        auto what = stage & access;
        if (what == access)
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
      checkStage(stage, AccessStage::DepthStencil, vk::PipelineStageFlagBits::eColorAttachmentOutput);
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
            .setSrcQueueFamilyIndex(buffer.before.queue_index)
            .setDstQueueFamilyIndex(buffer.after.queue_index)
            .setBuffer(vbuffer.native())
            .setOffset(0)
            .setSize(VK_WHOLE_SIZE));
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

          imagebar.emplace_back(vk::ImageMemoryBarrier()
            .setSrcAccessMask(translateAccessMask(image.before.stage, image.before.usage))
            .setDstAccessMask(translateAccessMask(image.after.stage, image.after.usage))
            .setSrcQueueFamilyIndex(image.before.queue_index)
            .setDstQueueFamilyIndex(image.after.queue_index)
            .setNewLayout(translateLayout(image.after.layout))
            .setOldLayout(translateLayout(image.before.layout))
            .setImage(vimage.native())
            .setSubresourceRange(range));
        }
        auto beforeStr = buildStageString(beforeStage);
        auto afterStr = buildStageString(afterStage);
        //F_ILOG("vkBarriers", "need to conjure some barriers: before:\"%s\" after:\"%s\"", beforeStr.c_str(), afterStr.c_str());
        auto vkBefore = conjureFlags(beforeStage);
        auto vkAfter = conjureFlags(afterStage);

        vk::ArrayProxy<const vk::BufferMemoryBarrier> buffers(bufferbar.size(), bufferbar.data());
        vk::ArrayProxy<const vk::ImageMemoryBarrier> images(imagebar.size(), imagebar.data());

        buffer.pipelineBarrier(vkBefore, vkAfter, {}, {}, buffers, images);
      }
    }

    void VulkanCommandBuffer::addCommands(VulkanDevice* device, vk::CommandBuffer buffer, CommandBuffer& list, BarrierSolver& solver)
    {
      int drawIndex = 0;
      int framebuffer = 0;
      ResourceHandle boundPipeline;
      for (auto iter = list.begin(); (*iter)->type != PacketType::EndOfPackets; iter++)
      {
        auto* header = *iter;
        //F_ILOG("addCommandsVK", "type header %d", header->type);
        addBarrier(device, buffer, solver.runBarrier(drawIndex));
        switch (header->type)
        {
          //        case CommandPacket::PacketType::BufferCopy:
          //        case CommandPacket::PacketType::Dispatch:
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
        case PacketType::RenderpassEnd:
        {
          buffer.endRenderPass();
          break;
        }
        default:
          break;
        }
        drawIndex++;
      }
    }

    void VulkanCommandBuffer::preprocess(VulkanDevice* device, backend::CommandBuffer& list)
    {
      backend::CommandBuffer::PacketHeader* rpbegin = nullptr;
      // find all renderpasses & compile all missing graphics pipelines
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
/*
      CommandPacket* beginPass = nullptr;
      CommandPacket* endPass = nullptr;

      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
        case CommandPacket::PacketType::RenderpassBegin:
        {
          F_ASSERT(beginPass == nullptr, "!?");
          beginPass = packet;
          break;
        }
        case CommandPacket::PacketType::RenderpassEnd:
        {
          F_ASSERT(beginPass != nullptr, "!?");
          endPass = packet;
          handleRenderpass(device, list, beginPass, endPass);
          beginPass = nullptr;
          endPass = nullptr;
          break;
        }
        case CommandPacket::PacketType::ComputePipelineBind:
        {
          auto& ref = packetRef(gfxpacket::ComputePipelineBind, packet);
          device->updatePipeline(ref.pipeline);
          // TODO: bind vulkan resources
          // TODO: 
          break;
        }
        default:
          break;
        }
      }
      */
    }
    // implementations
    void VulkanCommandBuffer::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, backend::CommandBuffer& list, BarrierSolver& solver)
    {
      auto nat = std::static_pointer_cast<VulkanDevice>(device);

      // F_ILOG("fillWith", "testing filling");
      // preprocess to compile renderpasses/pipelines

      preprocess(nat.get(), list);
      
      m_list->list().begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(nullptr));

      //addDepedencyDataAndSolve(solver, list);

      // add commands to list while also adding barriers
      addCommands(nat.get(), m_list->list(), list, solver);

      m_list->list().end();
    }
  }
}