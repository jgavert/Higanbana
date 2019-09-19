#pragma once
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include <higanbana/core/math/math.hpp>

namespace higanbana
{
  namespace gfxpacket
  {
    const char* packetTypeToString(backend::PacketType type);

    struct RenderBlock
    {
      backend::PacketVectorHeader<char> name;

      // constructors
      static constexpr const backend::PacketType type = backend::PacketType::RenderBlock;
      static void constructor(backend::CommandBuffer& buffer, RenderBlock* packet, MemView<char> inputName)
      {
        packet = buffer.allocateElements<char>(packet->name, inputName.size()+1, packet);
        auto spn = packet->name.convertToMemView();
        memcpy(spn.data(), inputName.data(), inputName.size_bytes());
        spn[inputName.size_bytes()] = '\0';
      }
    };

    struct ReleaseFromQueue
    {
      // constructors
      static constexpr const backend::PacketType type = backend::PacketType::ReleaseFromQueue;
      static void constructor(backend::CommandBuffer& buffer, ReleaseFromQueue* packet)
      {
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
      backend::PacketVectorHeader<float4> clearValues;
      ViewResourceHandle dsv;
      float clearDepth;

      int fbWidth;
      int fbHeight;

      static constexpr const backend::PacketType type = backend::PacketType::RenderpassBegin;
      static void constructor(backend::CommandBuffer& buffer, RenderPassBegin* packet, ResourceHandle renderpass, MemView<ViewResourceHandle> rtvs, ViewResourceHandle dsv, MemView<float4> clearVals, float clearDepth)
      {
        packet->renderpass = renderpass;
        packet = buffer.allocateElements<ViewResourceHandle>(packet->rtvs, rtvs.size(), packet);
        {
          auto spn = packet->rtvs.convertToMemView();
          memcpy(spn.data(), rtvs.data(), rtvs.size_bytes());
        }
        packet->dsv = dsv;
        packet = buffer.allocateElements<float4>(packet->clearValues, clearVals.size(), packet);
        {
          auto spn = packet->clearValues.convertToMemView();
          memcpy(spn.data(), clearVals.data(), clearVals.size_bytes());
        }
        packet->clearDepth = clearDepth;
      }
    };

    struct RenderPassEnd
    {
      static constexpr const backend::PacketType type = backend::PacketType::RenderpassEnd;
      static void constructor(backend::CommandBuffer& , RenderPassEnd* )
      {
      }
    };

    struct GraphicsPipelineBind
    {
      ResourceHandle pipeline;

      static constexpr const backend::PacketType type = backend::PacketType::GraphicsPipelineBind;
      static void constructor(backend::CommandBuffer& , GraphicsPipelineBind* packet, ResourceHandle pipeline)
      {
        packet->pipeline = pipeline;
      }
    };

    struct ComputePipelineBind
    {
      ResourceHandle pipeline;

      static constexpr const backend::PacketType type = backend::PacketType::ComputePipelineBind;
      static void constructor(backend::CommandBuffer& , ComputePipelineBind* packet, ResourceHandle pipeline)
      {
        packet->pipeline = pipeline;
      }
    };

    
    struct ResourceBinding
    {
      enum class BindingType : unsigned char
      {
        Graphics,
        Compute,
        Raytracing
      };

      BindingType graphicsBinding;
      backend::PacketVectorHeader<uint8_t> constants;
      backend::PacketVectorHeader<ViewResourceHandle> resources;

      static constexpr const backend::PacketType type = backend::PacketType::ResourceBinding;
      static void constructor(backend::CommandBuffer& buffer, ResourceBinding* packet, BindingType type, MemView<uint8_t>& constants, MemView<ShaderArguments>& views)
      {
        packet->graphicsBinding = type;
        packet = buffer.allocateElements<uint8_t>(packet->constants, constants.size(), packet);
        auto spn = packet->constants.convertToMemView();
        memcpy(spn.data(), constants.data(), constants.size_bytes());
        packet = buffer.allocateElements<ViewResourceHandle>(packet->resources, views.size(), packet);
        auto spn2 = packet->resources.convertToMemView();
        for (int i = 0; i < views.size(); ++i)
        {
          spn2[i] = views[i].handle();
        }
      }
    };

    struct Dispatch
    {
      uint3 groups;

      static constexpr const backend::PacketType type = backend::PacketType::Dispatch;
      static void constructor(backend::CommandBuffer& buffer, Dispatch* packet, uint3 groups)
      {
        packet->groups = groups;
        static_assert(std::is_standard_layout<Dispatch>::value, "this is trivial packet");
      }
    };

    struct Draw
    {
      uint32_t vertexCountPerInstance;
      uint32_t instanceCount;
      uint32_t startVertex;
      uint32_t startInstance;

      static constexpr const backend::PacketType type = backend::PacketType::Draw;
      static void constructor(backend::CommandBuffer& , Draw* packet, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
      {
        packet->vertexCountPerInstance = vertexCountPerInstance;
        packet->instanceCount = instanceCount;
        packet->startVertex = startVertex;
        packet->startInstance = startInstance;
      }
    };

    struct DrawIndexed
    {
      ViewResourceHandle indexbuffer;
      uint32_t IndexCountPerInstance;
      uint32_t instanceCount;
      uint32_t StartIndexLocation;
      int BaseVertexLocation;
      uint32_t StartInstanceLocation;

      static constexpr const backend::PacketType type = backend::PacketType::DrawIndexed;
      static void constructor(backend::CommandBuffer& , DrawIndexed* packet, ViewResourceHandle indexbuffer, uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLoc, int baseVertexLoc, uint32_t startInstance)
      {
        packet->indexbuffer = indexbuffer;
        packet->IndexCountPerInstance = indexCountPerInstance;
        packet->instanceCount = instanceCount;
        packet->StartIndexLocation = startIndexLoc;
        packet->BaseVertexLocation = baseVertexLoc;
        packet->StartInstanceLocation = startInstance;
      }
    };

    struct DynamicBufferCopy
    {
      ResourceHandle dst;
      uint32_t dstOffset;
      ViewResourceHandle src;
      uint32_t numBytes;

      static constexpr const backend::PacketType type = backend::PacketType::DynamicBufferCopy;
      static void constructor(backend::CommandBuffer& , DynamicBufferCopy* packet, ResourceHandle dst, uint32_t dstOffset, ViewResourceHandle src, uint32_t numBytes)
      {
        packet->dst = dst;
        packet->dstOffset = dstOffset;
        packet->src = src; 
        packet->numBytes = numBytes;
      }
    };

    struct BufferCopy
    {
      ResourceHandle dst;
      uint32_t dstOffset;
      ResourceHandle src;
      uint32_t srcOffset;
      uint32_t numBytes;

      static constexpr const backend::PacketType type = backend::PacketType::BufferCopy;
      static void constructor(backend::CommandBuffer& , BufferCopy* packet, ResourceHandle dst, uint32_t dstOffset, ResourceHandle src, uint32_t srcOffset, uint32_t numBytes)
      {
        packet->dst = dst;
        packet->dstOffset = dstOffset;
        packet->src = src; 
        packet->srcOffset = srcOffset; 
        packet->numBytes = numBytes;
      }
    };

    struct UpdateTexture
    {
      ResourceHandle tex;
      ViewResourceHandle dynamic;
      int allMips;
      int mip;
      int slice;
      int width;
      int height;

      static constexpr const backend::PacketType type = backend::PacketType::UpdateTexture;
      static void constructor(backend::CommandBuffer& , UpdateTexture* packet, ResourceHandle tex, ViewResourceHandle dynamic, int allMips, int mip, int slice, int width, int height)
      {
        packet->tex = tex;
        packet->dynamic = dynamic;
        packet->allMips = allMips;
        packet->mip = mip; 
        packet->slice = slice;
        packet->width = width;
        packet->height = height;
      }
    };

    struct ScissorRect
    {
      int2 topleft;
      int2 bottomright;

      static constexpr const backend::PacketType type = backend::PacketType::ScissorRect;
      static void constructor(backend::CommandBuffer& , ScissorRect* packet, int2 topleft, int2 bottomright)
      {
        packet->topleft = topleft;
        packet->bottomright = bottomright;
      }
    };

    struct ReadbackBuffer
    {
      ResourceHandle dst; // patched later when we know it.
      ResourceHandle src;
      uint32_t srcOffset;
      uint32_t numBytes;

      static constexpr const backend::PacketType type = backend::PacketType::ReadbackBuffer;
      static void constructor(backend::CommandBuffer& , ReadbackBuffer* packet, ResourceHandle src, uint64_t srcOffset, uint64_t numBytes)
      {
        packet->src = src; 
        packet->srcOffset = srcOffset; 
        packet->numBytes = numBytes;
        packet->dst = ResourceHandle(); // invalid handle for now
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