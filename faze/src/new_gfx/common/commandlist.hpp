#pragma once
#include "intermediatelist.hpp"
#include "buffer.hpp"
#include "texture.hpp"
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
  public:
    void clearRT(TextureRTV& rtv, vec4 color)
    {
      list.insert<gfxpacket::ClearRT>(rtv, color);
    }

    void prepareForPresent(TextureRTV& rtv, vec4 color)
    {
      list.insert<gfxpacket::ClearRT>(rtv, color);
    }
  };
}
