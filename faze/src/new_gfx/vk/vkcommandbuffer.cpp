#include "vkresources.hpp"
#include "util/VulkanDependencySolver.hpp"

namespace faze
{
  namespace backend
  {
    void handleRenderpass(VulkanDevice*, backend::IntermediateList&, CommandPacket* begin, CommandPacket* end)
    {
      F_LOG("yay\n");

      // step1. check if renderpass is done, otherwise create renderpass
      auto renderpassbegin = packetRef(gfxpacket::RenderpassBegin, begin);
      if (false)
      {
        for (CommandPacket* current = begin; current != end; current = current->nextPacket())
        {
        }

        vk::AttachmentDescription attach = vk::AttachmentDescription()
          .setFormat(vk::Format::eA1R5G5B5UnormPack16) // find from texture
          .setSamples(vk::SampleCountFlagBits::e1) // find from texture
          .setLoadOp(vk::AttachmentLoadOp::eLoad) // find from textureView
          .setStoreOp(vk::AttachmentStoreOp::eStore) // textureView
          .setStencilLoadOp(vk::AttachmentLoadOp::eLoad) // textureView
          .setStencilStoreOp(vk::AttachmentStoreOp::eStore) // textureView
          .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal) // choose based on usage, optimal. Maybe expose this to user.
          .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal); // probably best to keep it as it. Maybe expose this to user.

        vk::SubpassDescription description = vk::SubpassDescription()
          .setColorAttachmentCount(0)
          .setInputAttachmentCount(0)
          .setPreserveAttachmentCount(0)
          .setPColorAttachments(nullptr)
          .setPInputAttachments(nullptr)
          .setPPreserveAttachments(nullptr)
          .setPDepthStencilAttachment(nullptr)
          .setPResolveAttachments(nullptr)
          .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

        vk::SubpassDependency dependency = vk::SubpassDependency()
          .setSrcSubpass(0)
          .setDstSubpass(0)
          .setSrcStageMask(vk::PipelineStageFlagBits::eAllCommands)
          .setDstStageMask(vk::PipelineStageFlagBits::eAllCommands)
          .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead)
          .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        vk::RenderPassCreateInfo rpinfo = vk::RenderPassCreateInfo()
          .setAttachmentCount(0)
          .setDependencyCount(0)
          .setSubpassCount(0)
          .setPAttachments(nullptr)
          .setPDependencies(nullptr)
          .setPSubpasses(nullptr);
      }
      // step2. compile used pipelines against the renderpass (lel, later)
      // step3. collect and register framebuffer to renderpass
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