#include "graphics/common/commandpackets.hpp"

namespace faze
{
  namespace gfxpacket
  {
    std::string packetTypeToString(backend::CommandPacket::PacketType type)
    {
      switch (type)
      {
      case backend::CommandPacket::PacketType::BufferCopy:
        return "BufferCopy";
      case backend::CommandPacket::PacketType::Dispatch:
        return "Dispatch";
      case backend::CommandPacket::PacketType::ClearRT:
        return "ClearRT";
      case backend::CommandPacket::PacketType::PrepareForPresent:
        return "PrepareForPresent";
      case backend::CommandPacket::PacketType::RenderpassBegin:
        return "RenderpassBegin";
      case backend::CommandPacket::PacketType::RenderpassEnd:
        return "RenderpassEnd";
      case backend::CommandPacket::PacketType::Subpass:
        return "Subpass";
      case backend::CommandPacket::PacketType::GraphicsPipelineBind:
        return "GraphicsPipelineBind";
      case backend::CommandPacket::PacketType::ComputePipelineBind:
        return "ComputePipelineBind";
      case backend::CommandPacket::PacketType::ResourceBinding:
        return "ResourceBinding";
      case backend::CommandPacket::PacketType::Draw:
        return "Draw";
      default:
        return "Unknown";
      }
    }
  }
}