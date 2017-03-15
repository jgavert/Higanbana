#include "vkresources.hpp"
#include "util/VulkanDependencySolver.hpp"

namespace faze
{
  namespace backend
  {
    void handle(vk::CommandBuffer buffer, gfxpacket::ClearRT& packet)
    {
      auto view = std::static_pointer_cast<VulkanTextureView>(packet.rtv.native());
      auto texture = std::static_pointer_cast<VulkanTexture>(packet.rtv.texture().native());

      // TODO: texture->state()->flags[0].layout, figure out the index.

      buffer.clearColorImage(
        texture->native(),
        texture->state()->flags[0].layout,
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
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eColorAttachmentOutput);
          addTextureView(drawIndex, p.rtv, vk::ImageLayout::eGeneral, vk::AccessFlagBits::eTransferWrite);
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          auto& p = packetRef(gfxpacket::PrepareForPresent, packet);
          drawIndex = solver.addDrawCall(packet->type(), vk::PipelineStageFlagBits::eTopOfPipe);
          addTexture(drawIndex, p.texture, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eTransferWrite);
          drawIndex++;
        }
        default:
          break;
        }
      }
      solver.makeAllBarriers();
    }

    // implementations
    void VulkanCommandBuffer::fillWith(backend::IntermediateList& list)
    {
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