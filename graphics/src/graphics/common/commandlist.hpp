#pragma once
#include "graphics/common/intermediatelist.hpp"
#include "graphics/common/buffer.hpp"
#include "graphics/common/texture.hpp"
#include "graphics/common/renderpass.hpp"
#include "graphics/common/commandpackets.hpp"
#include "core/global_debug.hpp"

namespace faze
{
  class DmaList
  {
    friend class ComputeList;
    friend class GraphicsList;
    backend::IntermediateList list;
  public:
    DmaList()
    {
    }
  };

  class ComputeList : public DmaList
  {
    friend class GraphicsList;
  public:
    ComputeList()
      : DmaList()
    {}
  };

  class GraphicsList : public ComputeList
  {
  public:
    GraphicsList()
      : ComputeList()
    {}
  };

  class CommandList
  {
    backend::IntermediateList list;
    friend struct backend::DeviceGroupData;
  public:

    void renderTask(std::string name)
    {
      //list.insert<gfxpacket::RenderBlock>(name);
    }

    void updateTexture(Texture& tex, DynamicBufferView& dynBuffer, int mip, int slice)
    {
      //list.insert<gfxpacket::UpdateTexture>(tex, dynBuffer, mip, slice);
    }

    void clearRT(TextureRTV& rtv, float4 color)
    {
      //list.insert<gfxpacket::ClearRT>(rtv, color);
    }

    void prepareForPresent(TextureRTV& rtv)
    {
      //auto tex = rtv.texture();
      //list.insert<gfxpacket::PrepareForPresent>(tex);
    }

    void prepareForQueueSwitch(unordered_set<backend::TrackedState>& deps)
    {
      //list.insert<gfxpacket::PrepareForQueueSwitch>(deps);
    }

    void renderpass(Renderpass& pass, MemView<TextureRTV> rtvs, MemView<TextureDSV> dsvs)
    {
      //list.insert<gfxpacket::RenderpassBegin>(pass, rtvs, dsvs);
    }

    void renderpassEnd()
    {
      //list.insert<gfxpacket::RenderpassEnd>();
    }

    void bindPipeline(GraphicsPipeline& pipeline)
    {
      //list.insert<gfxpacket::GraphicsPipelineBind>(pipeline);
    }

    void bindPipeline(ComputePipeline& pipeline)
    {
      //list.insert<gfxpacket::ComputePipelineBind>(pipeline);
    }

    void bindGraphicsResources(
      MemView<ResourceHandle> resources,
      MemView<uint8_t> constants)
    {
      //list.insert<gfxpacket::ResourceBinding>(
      //  gfxpacket::ResourceBinding::BindingType::Graphics
      //  , resources, constants, views);
    }

    void bindComputeResources(
      MemView<ResourceHandle> resources,
      MemView<uint8_t> constants)
    {
      //list.insert<gfxpacket::ResourceBinding>(
      //  gfxpacket::ResourceBinding::BindingType::Compute,
      //  resources, constants, views);
    }

    void setScissorRect(int2 topLeft, int2 bottomRight)
    {
      //list.insert<gfxpacket::SetScissorRect>(topLeft, bottomRight);
    }

    void draw(
      unsigned vertexCountPerInstance,
      unsigned instanceCount,
      unsigned startVertex,
      unsigned startInstance)
    {
      //list.insert<gfxpacket::Draw>(vertexCountPerInstance, instanceCount, startVertex, startInstance);
    }

    void drawIndexed(
      BufferIBV& buffer,
      unsigned IndexCountPerInstance,
      unsigned instanceCount,
      unsigned StartIndexLocation,
      int BaseVertexLocation,
      unsigned StartInstanceLocation)
    {
      //list.insert<gfxpacket::DrawIndexed>(buffer, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void drawDynamicIndexed(
      DynamicBufferView& buffer,
      unsigned IndexCountPerInstance,
      unsigned instanceCount,
      unsigned StartIndexLocation,
      int BaseVertexLocation,
      unsigned StartInstanceLocation)
    {
      //list.insert<gfxpacket::DrawDynamicIndexed>(buffer, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void dispatch(
      uint3 groups)
    {
      //list.insert<gfxpacket::Dispatch>(groups);
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

    void copy(Buffer& target, Buffer& source)
    {
      //list.insert<gfxpacket::BufferCopy>(target.dependency(), source.dependency());
    }

    void copy(Buffer& target, DynamicBufferView& source)
    {
      //list.insert<gfxpacket::BufferCpuToGpuCopy>(target, source);
    }

    void readback(Texture& texture, Subresource range, Box srcbox, std::function<void(SubresourceData)> func)
    {
      //list.insert<gfxpacket::ReadbackTexture>(texture, range, srcbox, texture.desc().desc.format, func);
    }

    void readback(Buffer& buffer, uint startElement, uint size, std::function<void(MemView<uint8_t>)> func)
    {
      //list.insert<gfxpacket::Readback>(buffer, startElement * buffer.desc().desc.stride, size * buffer.desc().desc.stride, func);
    }

    void queryCounters(std::function<void(MemView<std::pair<std::string, double>>)> func)
    {
      //list.insert<gfxpacket::QueryCounters>(func);
    }
  };
}