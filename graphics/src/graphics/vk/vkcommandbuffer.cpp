#include "graphics/vk/vkresources.hpp"
#include "graphics/vk/vkdevice.hpp"
#include "graphics/vk/util/VulkanDependencySolver.hpp"
#include "core/datastructures/proxy.hpp"
#include <graphics/common/command_buffer.hpp>
namespace faze
{
  namespace backend
  {
/*
    void VulkanCommandBuffer::handleRenderpass(VulkanDevice* device, backend::IntermediateList&, CommandPacket* begin, CommandPacket* end)
    {
      // step1. check if renderpass is done, otherwise create renderpass
      auto& renderpassbegin = packetRef(gfxpacket::RenderpassBegin, begin);
      auto rp = std::static_pointer_cast<VulkanRenderpass>(renderpassbegin.renderpass.impl());
      if (!rp->valid())
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

        auto& subpass = packetRef(gfxpacket::RenderpassBegin, begin);
        // 1. find all the resources
        for (auto&& it : subpass.rtvs)
        {
          auto* found = uidToAttachmendId.find(it.id());
          if (found != uidToAttachmendId.end())
          {
            colors.emplace_back(vk::AttachmentReference()
              .setAttachment(found->second)
              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));
          }
          else
          {
            auto attachmentId = static_cast<int>(attachments.size());
            uidToAttachmendId[it.id()] = attachmentId;
            colors.emplace_back(vk::AttachmentReference()
              .setAttachment(attachmentId)
              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));

            auto view = std::static_pointer_cast<VulkanTextureView>(it.native());

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

            attachments.emplace_back(vk::AttachmentDescription()
              .setFormat(view->native().format)
              .setSamples(vk::SampleCountFlagBits::e1)
              .setLoadOp(load)
              .setStoreOp(store)
              .setStencilLoadOp(load)
              .setStencilStoreOp(store)
              .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
              .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal));
          }
        }

        if (subpass.dsvs.size() > 0)
        {
          auto& it = subpass.dsvs[0];
          auto* found = uidToAttachmendId.find(it.id());
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
            uidToAttachmendId[it.id()] = attachmentId;
            depthStencil.emplace_back(vk::AttachmentReference()
              .setAttachment(attachmentId)
              .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal));

            auto view = std::static_pointer_cast<VulkanTextureView>(it.native());

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

            attachments.emplace_back(vk::AttachmentDescription()
              .setFormat(view->native().format)
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

        rp->native() = rpobj;
      }
      // step2. compile used pipelines against the renderpass within begin/end packets
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

      // step3. collect and register framebuffer to renderpass
      unordered_map<int64_t, int> uidToAttachmendId;
      vector<vk::ImageView> attachments;

      int fbWidth = -1;
      int fbHeight = -1;

      auto& subpass = packetRef(gfxpacket::RenderpassBegin, begin);

      for (auto&& it : subpass.rtvs)
      {
        auto* found = uidToAttachmendId.find(it.id());
        if (found == uidToAttachmendId.end())
        {
          if (fbWidth == fbHeight && fbHeight == -1)
          {
            fbWidth = static_cast<int>(it.desc().desc.width);
            fbHeight = static_cast<int>(it.desc().desc.height);
          }
          F_ASSERT(fbWidth == static_cast<int>(it.desc().desc.width) && fbHeight == static_cast<int>(it.desc().desc.height), "Width and height must be same.");
          auto attachmentId = static_cast<int>(attachments.size());
          uidToAttachmendId[it.id()] = attachmentId;
          auto view = std::static_pointer_cast<VulkanTextureView>(it.native());
          attachments.emplace_back(view->native().view);
        }
      }
      if (subpass.dsvs.size() > 0)
      {
        // have dsv
        auto* found = uidToAttachmendId.find(subpass.dsvs[0].id());
        if (found == uidToAttachmendId.end())
        {
          if (fbWidth == fbHeight && fbHeight == -1)
          {
            fbWidth = static_cast<int>(subpass.dsvs[0].desc().desc.width);
            fbHeight = static_cast<int>(subpass.dsvs[0].desc().desc.height);
          }
          F_ASSERT(fbWidth == static_cast<int>(subpass.dsvs[0].desc().desc.width) && fbHeight == static_cast<int>(subpass.dsvs[0].desc().desc.height), "Width and height must be same.");
          auto attachmentId = static_cast<int>(attachments.size());
          uidToAttachmendId[subpass.dsvs[0].id()] = attachmentId;
          auto view = std::static_pointer_cast<VulkanTextureView>(subpass.dsvs[0].native());
          attachments.emplace_back(view->native().view);
        }
      }
      renderpassbegin.fbWidth = fbWidth;
      renderpassbegin.fbHeight = fbHeight;

      size_t attachmentsHash = HashMemory(attachments.data(), attachments.size());
      attachmentsHash = hash_combine(hash_combine(HashMemory(&fbWidth, sizeof(fbWidth)),HashMemory(&fbHeight, sizeof(fbHeight))), attachmentsHash);
      auto& fbs = rp->framebuffers();
      auto* exists = fbs.find(attachmentsHash);
      if (exists == fbs.end())
      {
        vk::FramebufferCreateInfo info = vk::FramebufferCreateInfo()
          .setWidth(static_cast<uint32_t>(fbWidth))
          .setHeight(static_cast<uint32_t>(fbHeight))
          .setLayers(1)
          .setPAttachments(attachments.data()) // imageview
          .setRenderPass(*rp->native())
          .setAttachmentCount(static_cast<uint32_t>(attachments.size()));

        fbs[attachmentsHash] = std::shared_ptr<vk::Framebuffer>(new vk::Framebuffer(device->native().createFramebuffer(info)), [dev = device->native()](vk::Framebuffer* ptr)
        {
          dev.destroyFramebuffer(*ptr);
          delete ptr;
        });
        F_LOG("Created a framebuffer\n");
      }
      else
      {
        //F_LOG("Framebuffer was already found!!!!\n");
      }
      rp->setActiveFramebuffer(attachmentsHash); // remember to set the current hash as active one.
    }

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

    void addCommands(VulkanDevice* device, vk::CommandBuffer buffer, backend::CommandBuffer& list)
    {
      int drawIndex = 0;
      for (auto iter = list.begin(); (*iter)->type != backend::PacketType::EndOfPackets; iter++)
      {
        auto* header = *iter;
        F_ILOG("addCommandsVK", "type header %d", header->type);
        switch (header->type)
        {
          //        case CommandPacket::PacketType::BufferCopy:
          //        case CommandPacket::PacketType::Dispatch:
        case backend::PacketType::PrepareForPresent:
        {
          //solver.runBarrier(buffer, drawIndex);
          drawIndex++;
          break;
        }
        case backend::PacketType::RenderpassBegin:
        {
          //solver.runBarrier(buffer, drawIndex);
          //drawIndex++;
          //handle(buffer, packetRef(gfxpacket::RenderpassBegin, packet));
          break;
        }
        case backend::PacketType::RenderpassEnd:
        {
          //buffer.endRenderPass();
          break;
        }
        default:
          break;
        }
      }
    }

    void VulkanCommandBuffer::preprocess(VulkanDevice* device, backend::CommandBuffer& list)
    {
      // find all renderpasses

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
    void VulkanCommandBuffer::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, backend::CommandBuffer& list)
    {
      auto nat = std::static_pointer_cast<VulkanDevice>(device);

      F_ILOG("fillWith", "testing filling");
      // preprocess to compile renderpasses/pipelines

      preprocess(nat.get(), list);
      
      m_list->list().begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(nullptr));

      //addDepedencyDataAndSolve(solver, list);

      // add commands to list while also adding barriers
      addCommands(nat.get(), m_list->list(), list);

      m_list->list().end();
    }
  }
}