#pragma once

#include "core/src/system/PageAllocator.hpp"
#include "core/src/datastructures/proxy.hpp"

namespace faze
{
  namespace backend
  {
    template <typename Heap>
    class HeapManager
    {
      struct HeapBlock
      {
        int index;
        PageAllocator allocator;
        Heap heap;
      };

      struct HeapVector
      {
        int alignment;
        vector<HeapBlock> heaps;
      };

      vector<HeapVector> m_heaps;
    public:

    };
  }
}