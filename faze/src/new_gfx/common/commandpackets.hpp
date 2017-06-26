#pragma once
#include "intermediatelist.hpp"
#include "commandvector.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "renderpass.hpp"
#include "core/src/math/vec_templated.hpp"

#define packetRef(type, packet) *static_cast<type*>(packet)
#define packetPtr(type, packet) static_cast<type*>(packet)

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

    // renderpass related packets

    class RenderpassBegin : public backend::CommandPacket
    {
    public:
      Renderpass renderpass;

      RenderpassBegin(backend::ListAllocator, Renderpass renderpass)
        : renderpass(renderpass)
      {
      }

      PacketType type() override
      {
        return PacketType::RenderpassBegin;
      }
    };

    class RenderpassEnd : public backend::CommandPacket
    {
    public:

      RenderpassEnd(backend::ListAllocator)
      {
      }

      PacketType type() override
      {
        return PacketType::RenderpassEnd;
      }
    };

    class Subpass : public backend::CommandPacket
    {
    public:
      CommandListVector<int> dependencies;
      CommandListVector<TextureRTV> rtvs;
      CommandListVector<TextureDSV> dsvs;
      Subpass(backend::ListAllocator allocator, MemView<int> inputDeps, MemView<TextureRTV> inputRtvs, MemView<TextureDSV> inputDsvs)
        : dependencies(MemView<int>(allocator.allocate<int>(inputDeps.size()), inputDeps.size()))
        , rtvs(MemView<TextureRTV>(allocator.allocate<TextureRTV>(inputRtvs.size()), inputRtvs.size()))
        , dsvs(MemView<TextureDSV>(allocator.allocate<TextureDSV>(inputDsvs.size()), inputDsvs.size()))
      {
        memcpy(dependencies.data(), inputDeps.data(), inputDeps.size());
        for (size_t i = 0; i < inputRtvs.size(); ++i)
        {
          rtvs[i] = inputRtvs[i];
        }
        for (size_t i = 0; i < inputDsvs.size(); ++i)
        {
          dsvs[i] = inputDsvs[i];
        }
      }

      PacketType type() override
      {
        return PacketType::Subpass;
      }
    };

    class GraphicsPipelineBind : public backend::CommandPacket
    {
    public:
      GraphicsPipeline pipeline;

      GraphicsPipelineBind(backend::ListAllocator, const GraphicsPipeline& pipeline)
        : pipeline(pipeline)
      {
      }

      PacketType type() override
      {
        return PacketType::GraphicsPipelineBind;
      }
    };

    class ComputePipelineBind : public backend::CommandPacket
    {
    public:
      ComputePipeline pipeline;

      ComputePipelineBind(backend::ListAllocator, const ComputePipeline& pipeline)
        : pipeline(pipeline)
      {
      }

      PacketType type() override
      {
        return PacketType::ComputePipelineBind;
      }
    };
  }
}