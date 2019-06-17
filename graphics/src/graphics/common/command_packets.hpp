#pragma once
#include <graphics/common/command_buffer.hpp>
#include <graphics/common/handle.hpp>

namespace faze
{
  namespace gfxpacket
  {
    std::string packetTypeToString(backend::PacketType type);

    struct RenderBlock
    {
      backend::PacketVectorHeader<char> name;

      // constructors
      static constexpr const backend::PacketType type = backend::PacketType::RenderBlock;
      static void constructor(backend::CommandBuffer& buffer, RenderBlock* packet, MemView<char> inputName)
      {
        buffer.allocateElements<char>(packet->name, inputName.size());
        auto spn = packet->name.convertToMemView();
        memcpy(spn.data(), inputName.data(), inputName.size_bytes());
      }
    };
    
    struct PrepareForPresent
    {
      ResourceHandle texture;

      static constexpr const backend::PacketType type = backend::PacketType::PrepareForPresent;
      static void constructor(backend::CommandBuffer& buffer, PrepareForPresent* packet, ResourceHandle texture)
      {
        packet->texture = texture;
      }
    };

    struct RenderPassBegin
    {
      ResourceHandle renderpass;
      backend::PacketVectorHeader<ViewResourceHandle> rtvs;
      ViewResourceHandle dsv;

      int fbWidth;
      int fbHeight;

      static constexpr const backend::PacketType type = backend::PacketType::RenderpassBegin;
      static void constructor(backend::CommandBuffer& buffer, RenderPassBegin* packet, ResourceHandle renderpass, MemView<ViewResourceHandle> rtvs, ViewResourceHandle dsv)
      {
        packet->renderpass = renderpass;
        buffer.allocateElements<ViewResourceHandle>(packet->rtvs, rtvs.size());
        auto spn = packet->rtvs.convertToMemView();
        memcpy(spn.data(), rtvs.data(), rtvs.size_bytes());
        packet->dsv = dsv;
      }
    };

    struct RenderPassEnd
    {
      static constexpr const backend::PacketType type = backend::PacketType::RenderpassEnd;
      static void constructor(backend::CommandBuffer& , RenderPassEnd* )
      {
      }
    };

    /*
    // helpers

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

    class RenderBlock : public backend::CommandPacket
    {
    public:
      std::string name;

      RenderBlock(backend::ListAllocator, std::string name)
        : name(name)
      {
      }

      PacketType type() override
      {
        return PacketType::RenderBlock;
      }
    };

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
      float4 color;

      ClearRT(backend::ListAllocator, TextureRTV rtv, float4 color)
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

    class PrepareForQueueSwitch : public backend::CommandPacket
    {
    public:
      unordered_set<backend::TrackedState> deps;

      PrepareForQueueSwitch(backend::ListAllocator, unordered_set<backend::TrackedState>& deps)
        : deps(deps)
      {
      }

      PacketType type() override
      {
        return PacketType::PrepareForQueueSwitch;
      }
    };

    // renderpass related packets

    class RenderpassBegin : public backend::CommandPacket
    {
    public:
      Renderpass renderpass;
      size_t hash;
      CommandListVector<TextureRTV> rtvs;
      CommandListVector<TextureDSV> dsvs;
      int fbWidth, fbHeight;

      RenderpassBegin(backend::ListAllocator allocator, Renderpass pass, MemView<TextureRTV> inputRtvs, MemView<TextureDSV> inputDsvs)
        : renderpass(pass)
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
      CommandListVector<backend::RawView> views;

      ResourceBinding(
        backend::ListAllocator allocator,
        BindingType graphicsBinding,
        MemView<backend::TrackedState>& resources,
        MemView<uint8_t>& constants,
        MemView<backend::RawView>& views)
        : graphicsBinding(graphicsBinding)
        , resources(toCmdVector(allocator, resources))
        , constants(toCmdVector(allocator, constants))
        , views(toCmdVector(allocator, views))
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

    class ReadbackTexture : public backend::CommandPacket
    {
    public:
      backend::TrackedState target;
      Subresource resource;
      Box srcbox;
      FormatType format;
      std::function<void(SubresourceData)> func;

      ReadbackTexture(backend::ListAllocator, Texture& target, Subresource resource, Box srcbox, FormatType format, std::function<void(SubresourceData)> func)
        : target(target.dependency())
        , resource(resource)
        , srcbox(srcbox)
        , format(format)
        , func(func)
      {
      }

      PacketType type() override
      {
        return PacketType::ReadbackTexture;
      }
    };

    class Readback : public backend::CommandPacket
    {
    public:
      backend::TrackedState target;
      uint64_t offset;
      uint64_t size;
      std::function<void(MemView<uint8_t>)> func;

      Readback(backend::ListAllocator, Buffer& target, uint64_t startIndex, uint64_t size, std::function<void(MemView<uint8_t>)> func)
        : target(target.dependency())
        , offset(startIndex)
        , size(size)
        , func(func)
      {
      }

      PacketType type() override
      {
        return PacketType::Readback;
      }
    };

    class QueryCounters : public backend::CommandPacket
    {
    public:
      std::function<void(MemView<std::pair<std::string, double>>)> func;

      QueryCounters(backend::ListAllocator, std::function<void(MemView<std::pair<std::string, double>>)>& func)
        : func(func)
      {
      }

      PacketType type() override
      {
        return PacketType::QueryCounters;
      }
    };

    class BufferCpuToGpuCopy : public backend::CommandPacket
    {
    public:
      backend::TrackedState target;
      uint64_t offset;
      uint64_t size;

      BufferCpuToGpuCopy(backend::ListAllocator, Buffer& target, DynamicBufferView& source)
        : target(target.dependency())
        , offset(static_cast<uint64_t>(source.native()->offset()))
        , size(std::min(static_cast<uint64_t>(target.desc().desc.width * target.desc().desc.stride), source.native()->size()))
      {
      }

      PacketType type() override
      {
        return PacketType::BufferCpuToGpuCopy;
      }
    };

    class TextureToBufferCopy : public backend::CommandPacket
    {
    public:
      backend::TrackedState target;
      backend::TrackedState source;
      int64_t targetoffset;
      Subresource subresource;
      Box srcbox;

      TextureToBufferCopy(backend::ListAllocator, backend::TrackedState target, int64_t targetOffset, backend::TrackedState source, Subresource subresource, Box srcbox)
        : target(target)
        , source(source)
        , targetoffset(targetOffset)
        , subresource(subresource)
        , srcbox(srcbox)
      {
      }

      PacketType type() override
      {
        return PacketType::TextureToBufferCopy;
      }
    };

    class TextureCopy : public backend::CommandPacket
    {
    public:
      backend::TrackedState target;
      backend::TrackedState source;
      SubresourceRange range;

      TextureCopy(backend::ListAllocator, backend::TrackedState target, backend::TrackedState source, SubresourceRange range)
        : target(target)
        , source(source)
        , range(range)
      {
      }

      PacketType type() override
      {
        return PacketType::TextureCopy;
      }
    };

    class TextureAdvCopy : public backend::CommandPacket
    {
    public:
      backend::TrackedState target;
      int3 dstPos;
      backend::TrackedState source;
      Subresource subresource;
      Box srcbox;

      TextureAdvCopy(backend::ListAllocator, backend::TrackedState target, int3 dst, backend::TrackedState source, Subresource subresource, Box srcbox)
        : target(target)
        , dstPos(dst)
        , source(source)
        , subresource(subresource)
        , srcbox(srcbox)
      {
      }

      PacketType type() override
      {
        return PacketType::TextureAdvCopy;
      }
    };

    class BufferCopy : public backend::CommandPacket
    {
    public:
      backend::TrackedState target;
      backend::TrackedState source;

      BufferCopy(backend::ListAllocator, backend::TrackedState target, backend::TrackedState source)
        : target(target)
        , source(source)
      {
      }

      PacketType type() override
      {
        return PacketType::BufferCopy;
      }
    };
    */
  }
}