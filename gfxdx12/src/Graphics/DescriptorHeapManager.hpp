#pragma once

#include "GpuResourceViewHeaper.hpp"
#include <array>

class DescriptorHeapManager 
{
private:
  enum HeapType
  {
    Generic = 0,
    RTV = 1,
    DSV = 2
  };
  friend class GpuDevice;
  friend class GraphicsQueue;
  friend class GfxCommandList;

  std::array<ResourceViewManager, 3> m_heaps;
  std::array<ID3D12DescriptorHeap*, 3> m_rawHeaps;
public:
  DescriptorHeapManager() {}

  ResourceViewManager& getGeneric()
  {
    return m_heaps[Generic];
  }
  ResourceViewManager& getSRV()
  {
    return m_heaps[Generic];
  }
  ResourceViewManager& getUAV()
  {
    return m_heaps[Generic];
  }
  ResourceViewManager& getRTV()
  {
    return m_heaps[RTV];
  }
  ResourceViewManager& getDSV()
  {
    return m_heaps[DSV];
  }

  ID3D12DescriptorHeap** getHeapPtr()
  {
    return m_rawHeaps.data();
  }

  unsigned getCount() // first 1 are safe to always bind to pipeline
  {
    return 1;
  }
};
