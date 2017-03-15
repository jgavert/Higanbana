#pragma once
#include "intermediatelist.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "core/src/math/vec_templated.hpp"

#define packetRef(type, packet) *static_cast<type*>(packet)

namespace faze
{
  namespace gfxpacket
  {
    std::string packetTypeToString(backend::CommandPacket::PacketType type);

    class ClearRT : public backend::CommandPacket
    {
    public:
      TextureRTV rtv;
      vec4 color;

      ClearRT(backend::ListAllocator, TextureRTV rtv, vec4 color)
        : rtv(rtv)
        , color(color)
      {
      }

      PacketType type() override
      {
        return PacketType::ClearRT;
      }
    };

    class PrepareForPresent : public backend::CommandPacket
    {
    public:
      Texture texture;

      PrepareForPresent(backend::ListAllocator, Texture& texture)
        :texture(texture)
      {
      }

      PacketType type() override
      {
        return PacketType::PrepareForPresent;
      }
    };
  }
}