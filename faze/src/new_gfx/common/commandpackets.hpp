#pragma once
#include "intermediatelist.hpp"
#include "core/src/math/vec_templated.hpp"

namespace faze
{
  namespace gfxpacket
  {
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