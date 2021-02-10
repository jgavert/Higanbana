#pragma once
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"
#include "higanbana/graphics/common/raytracing_descriptors.hpp"
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
      static void constructor(backend::CommandBuffer& buffer, RenderBlock& packet, MemView<char> inputName)
      {
        auto ptr = buffer.allocateElements<char>(packet.name, inputName.size()+1, packet);
        memcpy(ptr, inputName.data(), inputName.size_bytes());
        memset(ptr+inputName.size(), '\0', sizeof(char));
      }
    };

    struct ReleaseFromQueue
    {
      // constructors
      static constexpr const backend::PacketType type = backend::PacketType::ReleaseFromQueue;
      static void constructor(backend::CommandBuffer& buffer, ReleaseFromQueue& packet)
      {
      }
    };
    
    struct PrepareForPresent
    {
      ResourceHandle texture;

      static constexpr const backend::PacketType type = backend::PacketType::PrepareForPresent;
      static void constructor(backend::CommandBuffer& buffer, PrepareForPresent& packet, ResourceHandle texture)
      {
        packet.texture = texture;
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
      static void constructor(backend::CommandBuffer& buffer, RenderPassBegin& packet, ResourceHandle renderpass, MemView<ViewResourceHandle> rtvs, ViewResourceHandle dsv, MemView<float4> clearVals, float clearDepth)
      {
        packet.renderpass = renderpass;
        uint8_t* ptr = buffer.allocateElements<ViewResourceHandle>(packet.rtvs, rtvs.size(), packet);
        memcpy(ptr, rtvs.data(), rtvs.size_bytes());
        packet.dsv = dsv;
        ptr = buffer.allocateElements<float4>(packet.clearValues, clearVals.size(), packet);
        memcpy(ptr, clearVals.data(), clearVals.size_bytes());
        packet.clearDepth = clearDepth;
      }
    };

    struct RenderPassEnd
    {
      static constexpr const backend::PacketType type = backend::PacketType::RenderpassEnd;
      static void constructor(backend::CommandBuffer& , RenderPassEnd& )
      {
      }
    };

    struct GraphicsPipelineBind
    {
      ResourceHandle pipeline;

      static constexpr const backend::PacketType type = backend::PacketType::GraphicsPipelineBind;
      static void constructor(backend::CommandBuffer& , GraphicsPipelineBind& packet, ResourceHandle pipeline)
      {
        packet.pipeline = pipeline;
      }
    };

    struct ComputePipelineBind
    {
      ResourceHandle pipeline;

      static constexpr const backend::PacketType type = backend::PacketType::ComputePipelineBind;
      static void constructor(backend::CommandBuffer& , ComputePipelineBind& packet, ResourceHandle pipeline)
      {
        packet.pipeline = pipeline;
      }
    };

    struct ResourceBindingCompute
    {
      uint16_t constantsSize;
      uint16_t resourcesSize;

      static constexpr const backend::PacketType type = backend::PacketType::ResourceBindingCompute;
      static void constructor(backend::CommandBuffer& buffer, ResourceBindingCompute& packet, MemView<uint8_t>& constants, MemView<ShaderArguments>& views)
      {
        packet.constantsSize = static_cast<uint16_t>(constants.size());
        packet.resourcesSize = static_cast<uint16_t>(views.size());
        uint8_t* ptr = buffer.allocateElements2<uint8_t>(constants.size());
        memcpy(ptr, constants.data(), constants.size_bytes());
        ptr = buffer.allocateElements2<ResourceHandle>(views.size());
        for (int i = 0; i < views.size(); ++i)
        {
          auto h = views[i].handle();
          memcpy(ptr+i*sizeof(h), &h, sizeof(h));
        }
        static_assert(sizeof(ResourceBindingCompute) <= sizeof(uint32_t));
      }

      MemView<uint8_t> constantsView() const
      {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(reinterpret_cast<size_t>(this) + sizeof(ResourceBindingCompute));
        if (constantsSize == 0) return MemView<uint8_t>(nullptr, 0);
        return MemView<uint8_t>(ptr, constantsSize);
      }
      MemView<ResourceHandle> resourcesView() const
      {
        ResourceHandle* ptr = reinterpret_cast<ResourceHandle*>(reinterpret_cast<size_t>(this) + sizeof(ResourceBindingCompute) + constantsSize*sizeof(uint8_t));
        if (resourcesSize == 0) return MemView<ResourceHandle>(nullptr, 0);
        return MemView<ResourceHandle>(ptr, resourcesSize);
      }
    };
    
    struct ResourceBindingGraphics
    {
      uint16_t constantsSize;
      uint16_t resourcesSize;

      static constexpr const backend::PacketType type = backend::PacketType::ResourceBindingGraphics;
      static void constructor(backend::CommandBuffer& buffer, ResourceBindingGraphics& packet, MemView<uint8_t>& constants, MemView<ShaderArguments>& views)
      {
        packet.constantsSize = static_cast<uint16_t>(constants.size());
        packet.resourcesSize = static_cast<uint16_t>(views.size());
        uint8_t* ptr = buffer.allocateElements2<uint8_t>(constants.size());
        memcpy(ptr, constants.data(), constants.size_bytes());
        ptr = buffer.allocateElements2<ResourceHandle>(views.size());
        for (int i = 0; i < views.size(); ++i)
        {
          auto h = views[i].handle();
          memcpy(ptr+i*sizeof(h), &h, sizeof(h));
        }
        static_assert(sizeof(ResourceBindingGraphics) <= sizeof(uint32_t));
      }

      MemView<uint8_t> constantsView() const
      {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(reinterpret_cast<size_t>(this) + sizeof(ResourceBindingGraphics));
        if (constantsSize == 0) return MemView<uint8_t>(nullptr, 0);
        return MemView<uint8_t>(ptr, constantsSize);
      }
      MemView<ResourceHandle> resourcesView() const
      {
        ResourceHandle* ptr = reinterpret_cast<ResourceHandle*>(reinterpret_cast<size_t>(this) + sizeof(ResourceBindingGraphics) + constantsSize*sizeof(uint8_t));
        if (resourcesSize == 0) return MemView<ResourceHandle>(nullptr, 0);
        return MemView<ResourceHandle>(ptr, resourcesSize);
      }
    };

    struct Dispatch
    {
      uint3 groups;

      static constexpr const backend::PacketType type = backend::PacketType::Dispatch;
      static void constructor(backend::CommandBuffer& buffer, Dispatch& packet, uint3 groups)
      {
        packet.groups = groups;
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
      static void constructor(backend::CommandBuffer& , Draw& packet, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
      {
        packet.vertexCountPerInstance = vertexCountPerInstance;
        packet.instanceCount = instanceCount;
        packet.startVertex = startVertex;
        packet.startInstance = startInstance;
        static_assert(sizeof(Draw) <= sizeof(uint8_t)*16);
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
      static void constructor(backend::CommandBuffer& , DrawIndexed& packet, ViewResourceHandle indexbuffer, uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLoc, int baseVertexLoc, uint32_t startInstance)
      {
        packet.indexbuffer = indexbuffer;
        packet.IndexCountPerInstance = indexCountPerInstance;
        packet.instanceCount = instanceCount;
        packet.StartIndexLocation = startIndexLoc;
        packet.BaseVertexLocation = baseVertexLoc;
        packet.StartInstanceLocation = startInstance;
        static_assert(sizeof(DrawIndexed) <= sizeof(uint8_t)*40);
      }
    };

    struct DispatchMesh
    {
      uint xDim;

      static constexpr const backend::PacketType type = backend::PacketType::DispatchMesh;
      static void constructor(backend::CommandBuffer& , DispatchMesh& packet, uint xDim)
      {
        packet.xDim = xDim;
        static_assert(std::is_standard_layout<DispatchMesh>::value, "this is trivial packet");
      }
    };

    struct DynamicBufferCopy
    {
      ResourceHandle dst;
      uint32_t dstOffset;
      ViewResourceHandle src;
      uint32_t numBytes;

      static constexpr const backend::PacketType type = backend::PacketType::DynamicBufferCopy;
      static void constructor(backend::CommandBuffer& , DynamicBufferCopy& packet, ResourceHandle dst, uint32_t dstOffset, ViewResourceHandle src, uint32_t numBytes)
      {
        packet.dst = dst;
        packet.dstOffset = dstOffset;
        packet.src = src; 
        packet.numBytes = numBytes;
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
      static void constructor(backend::CommandBuffer& , BufferCopy& packet, ResourceHandle dst, uint32_t dstOffset, ResourceHandle src, uint32_t srcOffset, uint32_t numBytes)
      {
        packet.dst = dst;
        packet.dstOffset = dstOffset;
        packet.src = src; 
        packet.srcOffset = srcOffset; 
        packet.numBytes = numBytes;
      }
    };

    struct UpdateTexture
    {
      ResourceHandle tex;
      ViewResourceHandle dynamic;
      int3 dstPos;
      uint8_t mip;
      uint8_t slice;
      Box srcbox;

      static constexpr const backend::PacketType type = backend::PacketType::UpdateTexture;
      static void constructor(backend::CommandBuffer& , UpdateTexture& packet, ResourceHandle tex, Subresource dstSub, int3 dst, ViewResourceHandle dynamic, Box srcbox)
      {
        packet.tex = tex;
        packet.dynamic = dynamic;
        packet.dstPos = dst;
        packet.mip = dstSub.mipLevel;
        packet.slice = dstSub.arraySlice;
        packet.srcbox = srcbox;
      }
    };

    struct TextureToBufferCopy
    {
      ResourceHandle dstBuffer;
      uint32_t dstOffset;
      
      ResourceHandle srcTexture;
      uint32_t mip;
      uint32_t slice;
      uint32_t width;  
      uint32_t height;
      FormatType format;

      static constexpr const backend::PacketType type = backend::PacketType::TextureToBufferCopy;
      static void constructor(backend::CommandBuffer& , TextureToBufferCopy& packet,
        ResourceHandle dstBuffer, uint32_t dstOffset, 
        ResourceHandle srcTex, uint32_t mip, uint32_t slice, uint32_t width, uint32_t height, FormatType format)
      {
        packet.dstBuffer = dstBuffer;
        packet.dstOffset = dstOffset;
        packet.srcTexture = srcTex;
        packet.mip = mip;
        packet.slice = slice;
        packet.width = width;
        packet.height = height;
        packet.format = format;
      }
    };

    struct BufferToTextureCopy
    {
      ResourceHandle dstTexture;
      uint32_t mip;
      uint32_t slice;
      uint32_t width;  
      uint32_t height;
      FormatType format;

      ResourceHandle srcBuffer;
      uint32_t srcOffset;
      uint32_t numBytes; 

      static constexpr const backend::PacketType type = backend::PacketType::BufferToTextureCopy;
      static void constructor(backend::CommandBuffer& , BufferToTextureCopy& packet,
        ResourceHandle tex, uint32_t mip, uint32_t slice, uint32_t width, uint32_t height, FormatType format,
        ResourceHandle sourceBuffer, uint32_t srcOffset)
      {
        packet.dstTexture = tex;
        packet.mip = mip;
        packet.slice = slice;
        packet.width = width;
        packet.height = height;
        packet.format = format;
        packet.srcBuffer = sourceBuffer;
        packet.srcOffset = srcOffset;
      }
    };

    struct TextureToTextureCopy
    {
      ResourceHandle dst;
      ResourceHandle src;
      int3 dstPos;
      uint8_t dstMip;
      uint8_t dstSlice;
      uint8_t srcMip;
      uint8_t srcSlice;
      Box srcbox;

      static constexpr const backend::PacketType type = backend::PacketType::TextureToTextureCopy;
      static void constructor(backend::CommandBuffer& , TextureToTextureCopy& packet, ResourceHandle target, Subresource dstSub, int3 dst, ResourceHandle source, Subresource srcSub, Box srcbox)
      {
        packet.dst = target;
        packet.src = source;
        packet.dstPos = dst;
        packet.dstMip = dstSub.mipLevel;
        packet.dstSlice = dstSub.arraySlice;
        packet.srcMip = srcSub.mipLevel;
        packet.srcSlice = srcSub.arraySlice;
        packet.srcbox = srcbox;
      }
    };

    struct ScissorRect
    {
      int2 topleft;
      int2 bottomright;

      static constexpr const backend::PacketType type = backend::PacketType::ScissorRect;
      static void constructor(backend::CommandBuffer& , ScissorRect& packet, int2 topleft, int2 bottomright)
      {
        packet.topleft = topleft;
        packet.bottomright = bottomright;
      }
    };

    struct ReadbackTexture
    {
      ResourceHandle dst; // patched later when we know it. Buffer
      ResourceHandle src;
      uint32_t mip;
      uint32_t slice;
      Box srcbox;
      FormatType format;

      static constexpr const backend::PacketType type = backend::PacketType::ReadbackTexture;
      static void constructor(backend::CommandBuffer& , ReadbackTexture& packet, ResourceHandle src, Subresource srcSub, Box box, FormatType format)
      {
        packet.src = src; 
        packet.mip = srcSub.mipLevel;
        packet.slice = srcSub.arraySlice;
        packet.srcbox = box;
        packet.format = format;
        packet.dst = ResourceHandle(); // invalid handle for now
      }
    };

    struct ReadbackBuffer
    {
      ResourceHandle dst; // patched later when we know it.
      ResourceHandle src;
      uint32_t srcOffset;
      uint32_t numBytes;

      static constexpr const backend::PacketType type = backend::PacketType::ReadbackBuffer;
      static void constructor(backend::CommandBuffer& , ReadbackBuffer& packet, ResourceHandle src, uint64_t srcOffset, uint64_t numBytes)
      {
        packet.src = src; 
        packet.srcOffset = srcOffset; 
        packet.numBytes = numBytes;
        packet.dst = ResourceHandle(); // invalid handle for now
      }
    };

    struct RaytracingWriteGpuAddrToInstanceCPU
    {
      ViewResourceHandle dst;
      ResourceHandle addrToWrite;
      uint32_t instanceId;

      static constexpr const backend::PacketType type = backend::PacketType::RaytracingWriteGpuAddrToInstanceCPU;
      static void constructor(backend::CommandBuffer& , RaytracingWriteGpuAddrToInstanceCPU& packet, ViewResourceHandle dst, ResourceHandle addrToWrite, uint32_t instanceId )
      {
        packet.dst = dst; 
        packet.addrToWrite = addrToWrite; 
        packet.instanceId = instanceId;
      }
    };

    struct RaytracingWriteGpuAddrToInstanceGPU
    {
      ResourceHandle dst;
      ResourceHandle addrToWrite;
      uint32_t instanceId;

      static constexpr const backend::PacketType type = backend::PacketType::RaytracingWriteGpuAddrToInstanceGPU;
      static void constructor(backend::CommandBuffer& , RaytracingWriteGpuAddrToInstanceGPU& packet, ResourceHandle dst, ResourceHandle addrToWrite, uint32_t instanceId )
      {
        packet.dst = dst; 
        packet.addrToWrite = addrToWrite; 
        packet.instanceId = instanceId;
      }
    };

    struct BuildBLASTriangle
    {
      ResourceHandle dst;
      ResourceHandle scratch;
      backend::PacketVectorHeader<desc::RaytracingTriangleDescription> triangles;

      static constexpr const backend::PacketType type = backend::PacketType::BuildBLASTriangle;
      static void constructor(backend::CommandBuffer& buffer, BuildBLASTriangle& packet, ResourceHandle dst, desc::RaytracingAccelerationStructureInputs& asInputs, ResourceHandle scratch)
      {
        packet.dst = dst; 
        packet.scratch = scratch; 
        uint structs = asInputs.desc.triangles.size();
        uint bytes = structs * sizeof(desc::RaytracingTriangleDescription);
        uint8_t* ptr = buffer.allocateElements<desc::RaytracingTriangleDescription>(packet.triangles, structs, packet);
        //auto view = packet.triangles.convertToMemView();
        memcpy(ptr, &asInputs.desc.triangles[0], bytes);
        //memcpy(ptr, &asInputs.desc.triangles[0], bytes);
        //auto view = packet.triangles.convertToMemView();
        MemView<desc::RaytracingTriangleDescription> view = MemView<desc::RaytracingTriangleDescription>(reinterpret_cast<desc::RaytracingTriangleDescription*>(ptr), 1);
        HIGAN_ASSERT(view[0].indexBuffer == asInputs.desc.triangles[0].indexBuffer, "should be same");
      }
    };
    struct BuildTLAS
    {
      ResourceHandle scratch;
      ResourceHandle dst;

      static constexpr const backend::PacketType type = backend::PacketType::BuildTLAS;
      static void constructor(backend::CommandBuffer& , BuildTLAS& packet, ResourceHandle dst, desc::RaytracingAccelerationStructureInputs& asInputs, ResourceHandle scratch)
      {
        packet.dst = dst; 
        packet.scratch = scratch; 
      }
    };


    struct ReadbackShaderDebug
    {
      ResourceHandle dst;

      static constexpr const backend::PacketType type = backend::PacketType::ReadbackShaderDebug;
      static void constructor(backend::CommandBuffer& , ReadbackShaderDebug& packet, ResourceHandle dst)
      {
        packet.dst = dst;
      }
    };
    /*
    // helpers

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

    */
  }
}