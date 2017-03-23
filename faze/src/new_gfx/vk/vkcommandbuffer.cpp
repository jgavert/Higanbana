#include "vkresources.hpp"
#include "util/VulkanDependencySolver.hpp"

namespace faze
{
  namespace backend
  {
    void handleRenderpass(VulkanDevice* device, backend::IntermediateList&, CommandPacket* begin, CommandPacket* end)
    {
      F_LOG("yay\n");

      // step1. check if renderpass is done, otherwise create renderpass
      auto& renderpassbegin = packetRef(gfxpacket::RenderpassBegin, begin);
      auto rp = std::static_pointer_cast<VulkanRenderpass>(renderpassbegin.renderpass.impl());
      if (!rp->valid())
      {
        int subpassIndex = 0;

        unordered_map<int64_t, int> uidToAttachmendId;
        vector<vk::SubpassDependency> dependencies;
        vector<vk::SubpassDescription> subpasses;
        vector<vk::AttachmentDescription> attachments;

        // current subpass helpers
        vector<vk::AttachmentReference> colors;
        int colorsOffset = 0;
        vector<vk::AttachmentReference> depthStencil;
        vk::AttachmentReference* boundDsv = nullptr;
        bool startedSubpass = false;
        for (CommandPacket* current = begin; current != end; current = current->nextPacket())
        {
          if (current->type() == CommandPacket::PacketType::Subpass)
          {
            if (startedSubpass)
            {
              subpasses.emplace_back(vk::SubpassDescription()
                .setColorAttachmentCount(static_cast<int>(colors.size()) - colorsOffset)
                .setPColorAttachments(colors.data() + colorsOffset)
                .setPDepthStencilAttachment(boundDsv)
                .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics));
              colorsOffset = static_cast<int>(colors.size());
              boundDsv = nullptr;
            }
            startedSubpass = true;

            auto& subpass = packetRef(gfxpacket::Subpass, current);
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

            // 2. dependencies
            for (auto&& it : subpass.dependencies)
            {
              dependencies.emplace_back(vk::SubpassDependency()
                .setSrcSubpass(static_cast<uint32_t>(it))
                .setDstSubpass(static_cast<uint32_t>(subpassIndex))
                .setSrcStageMask(vk::PipelineStageFlagBits::eAllCommands)
                .setDstStageMask(vk::PipelineStageFlagBits::eAllCommands)
                .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead));
            }

            subpassIndex++;
          }
        }

        if (startedSubpass)
        {
          // handle last subpass here with renderpass creation.
          subpasses.emplace_back(vk::SubpassDescription()
            .setColorAttachmentCount(static_cast<int>(colors.size()))
            .setPColorAttachments(colors.data())
            .setPDepthStencilAttachment(boundDsv)
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics));
        }

        vk::RenderPassCreateInfo rpinfo = vk::RenderPassCreateInfo()
          .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
          .setDependencyCount(static_cast<uint32_t>(dependencies.size()))
          .setSubpassCount(static_cast<uint32_t>(subpasses.size()))
          .setPAttachments(attachments.data())
          .setPDependencies(dependencies.data())
          .setPSubpasses(subpasses.data());

        auto renderpass = device->native().createRenderPass(rpinfo);
        ;
        rp->native() = std::shared_ptr<vk::RenderPass>(new vk::RenderPass(renderpass), [dev = device->native()](vk::RenderPass* ptr)
        {
          dev.destroyRenderPass(*ptr);
          delete ptr;
        });

        F_LOG("have created a renderpass!\n");
      }
      // step2. compile used pipelines against the renderpass (lel, later)
      // step3. collect and register framebuffer to renderpass

      unordered_map<int64_t, int> uidToAttachmendId;
      vector<vk::ImageView> attachments;

      int fbWidth = -1;
      int fbHeight = -1;

      for (CommandPacket* current = begin; current != end; current = current->nextPacket())
      {
          if (current->type() == CommandPacket::PacketType::Subpass)
          {
              auto& subpass = packetRef(gfxpacket::Subpass, current);
              
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
          }
      }

      vk::FramebufferCreateInfo info = vk::FramebufferCreateInfo()
          .setWidth(static_cast<uint32_t>(fbWidth))
          .setHeight(static_cast<uint32_t>(fbHeight))
          .setLayers(1)
          .setPAttachments(attachments.data()) // imageview
          .setRenderPass(*rp->native())
          .setAttachmentCount(static_cast<uint32_t>(attachments.size()));

      auto framebuffer = device->native().createFramebuffer(info);
      device->native().destroyFramebuffer(framebuffer);
    }

    void preprocess(VulkanDevice* device, backend::IntermediateList& list)
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
          F_ASSERT(beginPass != nullptr && endPass == nullptr, "!?");
          endPass = packet;
          handleRenderpass(device, list, beginPass, endPass);
          beginPass = nullptr;
          endPass = nullptr;
          break;
        }
        default:
          break;
        }
      }
    }

    void handle(vk::CommandBuffer buffer, gfxpacket::ClearRT& packet)
    {
      auto view = std::static_pointer_cast<VulkanTextureView>(packet.rtv.native());
      auto texture = std::static_pointer_cast<VulkanTexture>(packet.rtv.texture().native());

      // TODO: texture->state()->flags[0].layout, figure out the index.

      buffer.clearColorImage(
        texture->native(),
        vk::ImageLayout::eTransferDstOptimal, // this probably should be somehow in the command, as we hold only old information in "texture"
        vk::ClearColorValue().setFloat32({ packet.color.x(), packet.color.y(), packet.color.z(), packet.color.w() }),
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
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          solver.runBarrier(buffer, drawIndex);
          drawIndex++;
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
        case CommandPacket::PacketType::ClearRT:
        {
          auto& p = packetRef(gfxpacket::ClearRT, packet);
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eTopOfPipe);
          addTextureView(drawIndex, p.rtv, vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite);
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          auto& p = packetRef(gfxpacket::PrepareForPresent, packet);
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eTopOfPipe);
          addTexture(drawIndex, p.texture, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eMemoryRead);
          drawIndex++;
        }
        default:
          break;
        }
      }
      solver.makeAllBarriers();
    }

    // implementations
    void VulkanCommandBuffer::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, backend::IntermediateList& list)
    {
      auto nat = std::static_pointer_cast<VulkanDevice>(device);

      // preprocess to compile renderpasses/pipelines

      preprocess(nat.get(), list);

      VulkanDependencySolver solver; // TODO: not really like this :D super heavy object, but might as well keep here until I know where it belongs to.
      m_cmdBuffer.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(nullptr));

      addDepedencyDataAndSolve(solver, list);

      // add commands to list while also adding barriers
      addCommands(m_cmdBuffer, solver, list);

      m_cmdBuffer.end();
    }
  }
}