#pragma once
#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/buffer.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/renderpass.hpp"
#include "higanbana/graphics/common/pipeline.hpp"
#include "higanbana/graphics/common/command_packets.hpp"
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
  class CommandList
  {
    backend::CommandBuffer list;
    friend struct backend::DeviceGroupData;
  public:
    CommandList()
      : list(256)
    {

    }

    CommandList(backend::CommandBuffer&& buffer)
      : list{std::move(buffer)}
    {
      list.reset();
    }

    void renderTask(std::string name)
    {
      list.insert<gfxpacket::RenderBlock>(name);
    }
/*
    void updateTexture(Texture& tex, DynamicBufferView& dynBuffer, int mip, int slice)
    {
      auto size = tex.desc().desc.size3D();
      size.x = size.x << mip;
      size.y = size.y << mip;
      list.insert<gfxpacket::UpdateTexture>(tex.handle(), dynBuffer.handle(), mip, slice, size.x, size.y);
    }*/

/*
    void clearRT(TextureRTV& rtv, float4 color)
    {
      //list.insert<gfxpacket::ClearRT>(rtv, color);
    }*/

    void prepareForPresent(TextureRTV& rtv)
    {
      auto tex = rtv.texture();
      list.insert<gfxpacket::PrepareForPresent>(tex.handle());
    }

/*
    void prepareForQueueSwitch(unordered_set<backend::TrackedState>& deps)
    {
      //list.insert<gfxpacket::PrepareForQueueSwitch>(deps);
    }*/

    void renderpass(Renderpass& pass, MemView<TextureRTV> rtvs, TextureDSV dsv)
    {
      vector<ViewResourceHandle> handles;
      for (auto&& rtv : rtvs)
      {
        handles.push_back(rtv.handle());
      }
      vector<float4> clearValues;
      for (auto&& rtv : rtvs)
      {
        clearValues.push_back(rtv.clearVal());
      }
      list.insert<gfxpacket::RenderPassBegin>(pass.handle(), handles, dsv.handle(), clearValues, dsv.clearVal().x, dsv.clearVal().y);
    }

    void renderpassEnd()
    {
      list.insert<gfxpacket::RenderPassEnd>();
    }

    void bindPipeline(GraphicsPipeline& pipeline)
    {
      list.insert<gfxpacket::GraphicsPipelineBind>(*pipeline.pipeline);
    }

    void bindPipeline(ComputePipeline& pipeline)
    {
      list.insert<gfxpacket::ComputePipelineBind>(*pipeline.impl);
    }

    void bindGraphicsResources(
      MemView<ShaderArguments> resources,
      MemView<uint8_t> constants,
      uint64_t constantBlock)
    {
      list.insert<gfxpacket::ResourceBindingGraphics>(
        constants, resources, constantBlock);
    }

    void bindComputeResources(
      MemView<ShaderArguments> resources,
      MemView<uint8_t> constants,
      uint64_t constantBlock)
    {
      list.insert<gfxpacket::ResourceBindingCompute>(
        constants, resources, constantBlock);
    }

    void setScissorRect(int2 topLeft, int2 bottomRight)
    {
      list.insert<gfxpacket::ScissorRect>(topLeft, bottomRight);
    }
    void setShadingRate(ShadingRate rate)
    {
      list.insert<gfxpacket::SetShadingRate>(rate);
    }

    void draw(
      unsigned vertexCountPerInstance,
      unsigned instanceCount,
      unsigned startVertex,
      unsigned startInstance)
    {
      list.insert<gfxpacket::Draw>(vertexCountPerInstance, instanceCount, startVertex, startInstance);
    }

    void drawIndexed(
      BufferIBV& buffer,
      unsigned IndexCountPerInstance,
      unsigned instanceCount,
      unsigned StartIndexLocation,
      int BaseVertexLocation,
      unsigned StartInstanceLocation)
    {
      list.insert<gfxpacket::DrawIndexed>(buffer.handle(), IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void drawDynamicIndexed(
      DynamicBufferView& buffer,
      unsigned IndexCountPerInstance,
      unsigned instanceCount,
      unsigned StartIndexLocation,
      int BaseVertexLocation,
      unsigned StartInstanceLocation)
    {
      list.insert<gfxpacket::DrawIndexed>(buffer.handle(), IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void dispatch(
      uint3 groups)
    {
      list.insert<gfxpacket::Dispatch>(groups);
    }

    void dispatchMesh(uint xGroups)
    {
      list.insert<gfxpacket::DispatchMesh>(xGroups);
    }

    void drawIndirect(uint maxCommands, const BufferSRV& indirectParams, const BufferSRV& count, uint countOffsetBytes) {
      list.insert<gfxpacket::DrawIndirect>(maxCommands, indirectParams.handle(), count.handle(), countOffsetBytes);
    }
    void drawIndexedIndirect(DynamicBufferView& buffer, uint maxCommands, const BufferSRV& indirectParams, const BufferSRV& count, uint countOffsetBytes) {
      list.insert<gfxpacket::DrawIndexedIndirect>(buffer.handle(), maxCommands, indirectParams.handle(), count.handle(), countOffsetBytes);
    }
    void drawIndexedIndirect(const BufferIBV& buffer, uint maxCommands, const BufferSRV& indirectParams, const BufferSRV& count, uint countOffsetBytes) {
      list.insert<gfxpacket::DrawIndexedIndirect>(buffer.handle(), maxCommands, indirectParams.handle(), count.handle(), countOffsetBytes);
    }
    void dispatchIndirect(const BufferSRV& indirectParams) {
      list.insert<gfxpacket::DispatchIndirect>(indirectParams.handle());
    }
    void dispatchRaysIndirect(uint maxCommands, const BufferSRV& indirectParams, const BufferSRV& count, uint countOffsetBytes) {
      list.insert<gfxpacket::DispatchRaysIndirect>(maxCommands, indirectParams.handle(), count.handle(), countOffsetBytes);
    }
    void dispatchMeshIndirect(uint maxCommands, const BufferSRV& indirectParams, const BufferSRV& count, uint countOffsetBytes) {
      list.insert<gfxpacket::DispatchMeshIndirect>(maxCommands, indirectParams.handle(), count.handle(), countOffsetBytes);
    }

    void updateTexture(Texture& tex, Subresource dstSubresource, int3 dstPos, DynamicBufferView& dynBuffer, Box srcbox)
    {
      list.insert<gfxpacket::UpdateTexture>(tex.handle(), dstSubresource, dstPos, dynBuffer.handle(), srcbox);
    }


    void copy(Buffer& target, int64_t targetOffset, Texture& source, Subresource subresource, Box srcbox)
    {
      HIGAN_ASSERT(false, "");
      //list.insert<gfxpacket::TextureToBufferCopy>(target.dependency(), targetOffset, source.dependency(), subresource, srcbox);
    }

    void copy(Texture& target, Subresource dstSubresource, int3 dstPos, Texture& source, Subresource srcSubresource, Box srcbox)
    {
      list.insert<gfxpacket::TextureToTextureCopy>(target.handle(), dstSubresource, dstPos, source.handle(), srcSubresource, srcbox);
    }


    void copy(Texture& target, Texture& source, SubresourceRange range)
    {
      HIGAN_ASSERT(false, "");
      //list.insert<gfxpacket::TextureCopy>(target.dependency(), source.dependency(), range);
    }
    

    void copy(Buffer target, size_t targetOffsetBytes, Texture source, Subresource sub) {
      auto size = source.desc().desc.size3D().xy();
      auto type = source.desc().desc.format;
      list.insert<gfxpacket::TextureToBufferCopy>(target.handle(), targetOffsetBytes, source.handle(), sub.mipLevel, sub.arraySlice, size.x, size.y, type);
    }

    void copy(Texture target, Subresource sub, Buffer source, size_t srcOffsetBytes) {
      auto size = target.desc().desc.size3D().xy();
      auto type = target.desc().desc.format;
      list.insert<gfxpacket::BufferToTextureCopy>(target.handle(),  sub.mipLevel, sub.arraySlice, size.x, size.y, type, source.handle(), srcOffsetBytes);
    }

    void copy(Buffer& target, uint dstStart, Buffer& source, uint srcStart, uint elements)
    {
      auto dstOffset = dstStart * target.desc().desc.stride;
      auto srcOffset = srcStart * source.desc().desc.stride;
      auto bytes = elements*source.desc().desc.stride;
      list.insert<gfxpacket::BufferCopy>(target.handle(), dstOffset, source.handle(), srcOffset, bytes);
    }

    void copy(Buffer& target, uint dstStart, DynamicBufferView& source, uint srcStart, uint elements)
    {
      auto dstOffset = dstStart * target.desc().desc.stride;
      auto srcOffset = srcStart * source.elementSize();
      auto bytes = source.elementSize()*elements;
      list.insert<gfxpacket::DynamicBufferCopy>(target.handle(), dstOffset, source.handle(), source.logicalSize());
    }

    void readback(Texture& texture, Subresource range, Box srcbox)
    {
      list.insert<gfxpacket::ReadbackTexture>(texture.handle(), range, srcbox, texture.desc().desc.format);
    }

    void readback(Buffer& buffer, uint byteOffset, uint sizeBytes)
    {
      list.insert<gfxpacket::ReadbackBuffer>(buffer.handle(), byteOffset, sizeBytes);
    }

    void raytracingWriteGPUAddrToInstanceDescCPU(DynamicBufferView& dst, const BufferRTAS& addrToWrite, uint instanceIndex) {
      list.insert<gfxpacket::RaytracingWriteGpuAddrToInstanceCPU>(dst.handle(), addrToWrite.buffer().handle(), instanceIndex);
    }
    void raytracingWriteGPUAddrToInstanceDescGPU(Buffer& dst, const BufferRTAS& addrToWrite, uint instanceIndex) {
      list.insert<gfxpacket::RaytracingWriteGpuAddrToInstanceGPU>(dst.handle(), addrToWrite.buffer().handle(), instanceIndex);
    }
    void buildBottomLevelAccelerationStructure(BufferRTAS& dst, desc::RaytracingAccelerationStructureInputs& asInputs, Buffer& scratch) {
      list.insert<gfxpacket::BuildBLASTriangle>(dst.buffer().handle(), asInputs, scratch.handle());
    }
    void buildTopLevelAccelerationStructure(BufferRTAS& dst, desc::RaytracingAccelerationStructureInputs& asInputs, Buffer& scratch) {
      list.insert<gfxpacket::BuildTLAS>(dst.buffer().handle(), asInputs, scratch.handle());
    }

    size_t sizeBytesUsed() const noexcept
    {
      return list.sizeBytes();
    }
  };

  class CommandBufferPool
  {
    vector<backend::CommandBuffer> buffers;
    std::mutex m_bufferLock;
    public:
    CommandList allocate()
    {
      std::lock_guard<std::mutex> lock(m_bufferLock);
      if (buffers.empty())
        return backend::CommandBuffer();
      backend::CommandBuffer buffer = std::move(buffers.back());
      buffers.pop_back();
      return buffer;
    }
    std::lock_guard<std::mutex> lock() {
      return std::lock_guard<std::mutex>(m_bufferLock);
    }
    void free(backend::CommandBuffer&& buffer)
    {
      buffers.emplace_back(std::move(buffer));
    }
  };
}