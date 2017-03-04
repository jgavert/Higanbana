#pragma once
#include "intermediatelist.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  static constexpr const int CommandlistSize = 1024 * 1024;

  class DmaList
  {
    friend class ComputeList;
    friend class GraphicsList;
    backend::IntermediateList list;
  public:
    DmaList()
      : list(LinearAllocator(CommandlistSize))
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
}
