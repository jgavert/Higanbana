#include "graphics/common/command_packets.hpp"

namespace faze
{
  namespace gfxpacket
  {
    std::string packetTypeToString(backend::PacketType type)
    {
      switch (type)
      {
      case backend::PacketType::BufferCopy:
        return "BufferCopy";
      case backend::PacketType::Dispatch:
        return "Dispatch";
      case backend::PacketType::PrepareForPresent:
        return "PrepareForPresent";
      case backend::PacketType::RenderpassBegin:
        return "RenderpassBegin";
      case backend::PacketType::RenderpassEnd:
        return "RenderpassEnd";
      case backend::PacketType::GraphicsPipelineBind:
        return "GraphicsPipelineBind";
      case backend::PacketType::ComputePipelineBind:
        return "ComputePipelineBind";
      case backend::PacketType::ResourceBinding:
        return "ResourceBinding";
      case backend::PacketType::Draw:
        return "Draw";
      default:
        return "Unknown";
      }
    }
  }
}