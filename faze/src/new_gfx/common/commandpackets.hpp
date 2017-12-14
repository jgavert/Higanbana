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
    // helpers
    std::string packetTypeToString(backend::CommandPacket::PacketType type);

    template <typename Type>
    CommandListVector<Type> toCmdVector(backend::ListAllocator& allocator, MemView<Type>& view)
    {
      if (view.size() == 0)
        return CommandListVector<Type>();

      CommandListVector<Type> fnl(MemView<Type>(allocator.allocate<Type>(view.size()), view.size()));

      if (std::is_pod<Type>::value)
      {
        memcpy(fnl.data(), view.data(), view.size() * sizeof(Type));
      }
      else
      {
        for (size_t i = 0; i < view.size(); ++i)
        {
          fnl[i] = view[i];
        }
      }

      return fnl;
    }

    // packets

    class UpdateTexture : public backend::CommandPacket
    {
    public:
      Texture dst;
      DynamicBufferView src;
      int mip;
      int slice;

      UpdateTexture(backend::ListAllocator, Texture dst, DynamicBufferView view, int mip, int slice)
        : dst(dst)
        , src(view)
        , mip(mip)
        , slice(slice)
      {

      }

      PacketType type() override
      {
        return PacketType::UpdateTexture;
      }
    };

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
      size_t hash;
      CommandListVector<int> dependencies;
      CommandListVector<TextureRTV> rtvs;
      CommandListVector<TextureDSV> dsvs;

      Subpass(backend::ListAllocator allocator, MemView<int> inputDeps, MemView<TextureRTV> inputRtvs, MemView<TextureDSV> inputDsvs)
        : dependencies(toCmdVector(allocator, inputDeps))
        , rtvs(toCmdVector(allocator, inputRtvs))
        , dsvs(toCmdVector(allocator, inputDsvs))
      {
        std::vector<FormatType> views;
        for (auto&& it : rtvs)
        {
          views.emplace_back(it.format());
        }
        for (auto&& it : dsvs)
        {
          views.emplace_back(it.format());
        }
        hash = HashMemory(views.data(), views.size() * sizeof(FormatType));
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

    class ResourceBinding : public backend::CommandPacket
    {
    public:
      enum class BindingType : unsigned char
      {
        Graphics,
        Compute
      };

      BindingType graphicsBinding;
      CommandListVector<backend::TrackedState> resources;
      CommandListVector<uint8_t> constants;
      CommandListVector<backend::RawView> srvs;
      CommandListVector<backend::RawView> uavs;

      ResourceBinding(
        backend::ListAllocator allocator,
        BindingType graphicsBinding,
        MemView<backend::TrackedState>& resources,
        MemView<uint8_t>& constants,
        MemView<backend::RawView>& srvs,
        MemView<backend::RawView>& uavs)
        : graphicsBinding(graphicsBinding)
        , resources(toCmdVector(allocator, resources))
        , constants(toCmdVector(allocator, constants))
        , srvs(toCmdVector(allocator, srvs))
        , uavs(toCmdVector(allocator, uavs))
      {
      }

      PacketType type() override
      {
        return PacketType::ResourceBinding;
      }
    };

	class SetScissorRect : public backend::CommandPacket
	{
	public:
		int2 topleft;
		int2 bottomright;

		SetScissorRect(backend::ListAllocator, int2 topleft, int2 bottomright)
			: topleft(topleft)
			, bottomright(bottomright)
		{
		}

		PacketType type() override
		{
			return PacketType::SetScissorRect;
		}
	};

    class Draw : public backend::CommandPacket
    {
    public:
      unsigned vertexCountPerInstance;
      unsigned instanceCount;
      unsigned startVertex;
      unsigned startInstance;

      Draw(backend::ListAllocator,
        unsigned vertexCountPerInstance,
        unsigned instanceCount,
        unsigned startVertex,
        unsigned startInstance)
        : vertexCountPerInstance(vertexCountPerInstance)
        , instanceCount(instanceCount)
        , startVertex(startVertex)
        , startInstance(startInstance)
      {
      }

      PacketType type() override
      {
        return PacketType::Draw;
      }
    };

	class DrawIndexed : public backend::CommandPacket
	{
	public:
		BufferIBV ib;
		unsigned IndexCountPerInstance;
		unsigned instanceCount;
		unsigned StartIndexLocation;
		int BaseVertexLocation;
		unsigned StartInstanceLocation;

		DrawIndexed(backend::ListAllocator,
			BufferIBV ib,
			unsigned IndexCountPerInstance,
			unsigned instanceCount,
			unsigned StartIndexLocation,
			int BaseVertexLocation,
			unsigned StartInstanceLocation)
			: ib(ib)
			, IndexCountPerInstance(IndexCountPerInstance)
			, instanceCount(instanceCount)
			, StartIndexLocation(StartIndexLocation)
			, BaseVertexLocation(BaseVertexLocation)
			, StartInstanceLocation(StartInstanceLocation)
		{
		}

		PacketType type() override
		{
			return PacketType::DrawIndexed;
		}
	};

	class DrawDynamicIndexed : public backend::CommandPacket
	{
	public:
		DynamicBufferView ib;
		unsigned IndexCountPerInstance;
		unsigned instanceCount;
		unsigned StartIndexLocation;
		int BaseVertexLocation;
		unsigned StartInstanceLocation;

		DrawDynamicIndexed(backend::ListAllocator,
			DynamicBufferView ib,
			unsigned IndexCountPerInstance,
			unsigned instanceCount,
			unsigned StartIndexLocation,
			int BaseVertexLocation,
			unsigned StartInstanceLocation)
			: ib(ib)
			, IndexCountPerInstance(IndexCountPerInstance)
			, instanceCount(instanceCount)
			, StartIndexLocation(StartIndexLocation)
			, BaseVertexLocation(BaseVertexLocation)
			, StartInstanceLocation(StartInstanceLocation)
		{
		}

		PacketType type() override
		{
			return PacketType::DrawDynamicIndexed;
		}
	};



    class Dispatch : public backend::CommandPacket
    {
    public:
      uint3 groups;

      Dispatch(backend::ListAllocator, uint3 groups)
        : groups(groups)
      {
      }

      PacketType type() override
      {
        return PacketType::Dispatch;
      }
    };
  }
}