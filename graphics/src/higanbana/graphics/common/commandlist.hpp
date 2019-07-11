#pragma once
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

    void renderTask(std::string name)
    {
      list.insert<gfxpacket::RenderBlock>(name);
    }

    void updateTexture(Texture& tex, DynamicBufferView& dynBuffer, int mip, int slice)
    {
      auto size = tex.desc().desc.size3D();
      size.x = size.x << mip;
      size.y = size.y << mip;
      list.insert<gfxpacket::UpdateTexture>(tex.handle(), dynBuffer.handle(), tex.desc().desc.miplevels, mip, slice, size.x, size.y);
    }

    void clearRT(TextureRTV& rtv, float4 color)
    {
      //list.insert<gfxpacket::ClearRT>(rtv, color);
    }

    void prepareForPresent(TextureRTV& rtv)
    {
      auto tex = rtv.texture();
      list.insert<gfxpacket::PrepareForPresent>(tex.handle());
    }

    void prepareForQueueSwitch(unordered_set<backend::TrackedState>& deps)
    {
      //list.insert<gfxpacket::PrepareForQueueSwitch>(deps);
    }

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
      list.insert<gfxpacket::RenderPassBegin>(pass.handle(), handles, dsv.handle(), clearValues, dsv.clearVal().x);
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
      MemView<ViewResourceHandle> resources,
      MemView<uint8_t> constants)
    {
      list.insert<gfxpacket::ResourceBinding>(
        gfxpacket::ResourceBinding::BindingType::Graphics
        , constants, resources);
    }

    void bindComputeResources(
      MemView<ViewResourceHandle> resources,
      MemView<uint8_t> constants)
    {
      list.insert<gfxpacket::ResourceBinding>(
        gfxpacket::ResourceBinding::BindingType::Compute
        , constants, resources);
    }

    void setScissorRect(int2 topLeft, int2 bottomRight)
    {
      list.insert<gfxpacket::ScissorRect>(topLeft, bottomRight);
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

    void copy(Buffer& target, int64_t targetOffset, Texture& source, Subresource subresource, Box srcbox)
    {
      //list.insert<gfxpacket::TextureToBufferCopy>(target.dependency(), targetOffset, source.dependency(), subresource, srcbox);
    }

    void copy(Texture& target, int3 dstPos, Texture& source, Subresource subresource, Box srcbox)
    {
      //list.insert<gfxpacket::TextureAdvCopy>(target.dependency(), dstPos, source.dependency(), subresource, srcbox);
    }

    void copy(Texture& target, Texture& source, SubresourceRange range)
    {
      //list.insert<gfxpacket::TextureCopy>(target.dependency(), source.dependency(), range);
    }

    void copy(Buffer& target, uint dstStart, Buffer& source, uint srcStart, uint elements)
    {
      auto dstOffset = dstStart * target.desc().desc.stride;
      auto srcOffset = srcStart * source.desc().desc.stride;
      auto bytes = source.desc().desc.sizeBytesBuffer();
      list.insert<gfxpacket::BufferCopy>(target.handle(), dstOffset, source.handle(), srcOffset, bytes);
    }

    void copy(Buffer& target, uint dstStart, DynamicBufferView& source)
    {
      auto dstOffset = dstStart * target.desc().desc.stride;
      list.insert<gfxpacket::DynamicBufferCopy>(target.handle(), dstOffset, source.handle(), source.logicalSize());
    }

    void readback(Texture& texture, Subresource range, Box srcbox)
    {
      //list.insert<gfxpacket::ReadbackTexture>(texture, range, srcbox, texture.desc().desc.format);
    }

    void readback(Buffer& buffer, uint startElement, uint size)
    {
      list.insert<gfxpacket::ReadbackBuffer>(buffer.handle(), startElement * buffer.desc().desc.stride, size * buffer.desc().desc.stride);
    }

    void queryCounters(std::function<void(MemView<std::pair<std::string, double>>)> func)
    {
      //list.insert<gfxpacket::QueryCounters>(func);
    }
  };
}