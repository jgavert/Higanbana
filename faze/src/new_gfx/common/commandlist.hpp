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
    void clearRT(TextureRTV& rtv, vec4 color)
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
  };
}