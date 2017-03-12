#include "commandpackets.hpp"

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
      default:
        return "Unknown";
      }
    }
  }
}