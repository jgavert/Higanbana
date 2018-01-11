#pragma once
#include "intermediatelist.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "renderpass.hpp"
#include "commandpackets.hpp"
#include "core/src/global_debug.hpp"

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
    friend struct backend::DeviceData;
  public:

    void updateTexture(Texture& tex, DynamicBufferView& dynBuffer, int mip, int slice)
    {
      list.insert<gfxpacket::UpdateTexture>(tex, dynBuffer, mip, slice);
    }

    void clearRT(TextureRTV& rtv, float4 color)
    {
      list.insert<gfxpacket::ClearRT>(rtv, color);
    }

    void prepareForPresent(TextureRTV& rtv)
    {
      auto tex = rtv.texture();
      list.insert<gfxpacket::PrepareForPresent>(tex);
    }

    void renderpass(Renderpass& pass)
    {
      list.insert<gfxpacket::RenderpassBegin>(pass);
    }

    void renderpassEnd()
    {
      list.insert<gfxpacket::RenderpassEnd>();
    }

    void subpass(MemView<int> deps, MemView<TextureRTV> rtvs, MemView<TextureDSV> dsvs)
    {
      list.insert<gfxpacket::Subpass>(deps, rtvs, dsvs);
    }

    void bindPipeline(GraphicsPipeline& pipeline)
    {
      list.insert<gfxpacket::GraphicsPipelineBind>(pipeline);
    }

    void bindPipeline(ComputePipeline& pipeline)
    {
      list.insert<gfxpacket::ComputePipelineBind>(pipeline);
    }

    void bindGraphicsResources(
      MemView<backend::TrackedState> resources,
      MemView<uint8_t> constants,
      MemView<backend::RawView> srvs,
      MemView<backend::RawView> uavs)
    {
      list.insert<gfxpacket::ResourceBinding>(
        gfxpacket::ResourceBinding::BindingType::Graphics
        , resources, constants, srvs, uavs);
    }

    void bindComputeResources(
      MemView<backend::TrackedState> resources,
      MemView<uint8_t> constants,
      MemView<backend::RawView> srvs,
      MemView<backend::RawView> uavs)
    {
      list.insert<gfxpacket::ResourceBinding>(
        gfxpacket::ResourceBinding::BindingType::Compute,
        resources, constants, srvs, uavs);
    }

    void setScissorRect(int2 topLeft, int2 bottomRight)
    {
      list.insert<gfxpacket::SetScissorRect>(topLeft, bottomRight);
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
      list.insert<gfxpacket::DrawIndexed>(buffer, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void drawDynamicIndexed(
      DynamicBufferView& buffer,
      unsigned IndexCountPerInstance,
      unsigned instanceCount,
      unsigned StartIndexLocation,
      int BaseVertexLocation,
      unsigned StartInstanceLocation)
    {
      list.insert<gfxpacket::DrawDynamicIndexed>(buffer, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void dispatch(
      uint3 groups)
    {
      list.insert<gfxpacket::Dispatch>(groups);
    }

    void copy(Buffer& target, Buffer& source)
    {
      list.insert<gfxpacket::BufferCopy>(target, source);
    }
  };
}