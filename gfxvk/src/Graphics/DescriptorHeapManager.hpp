#pragma once
#include "GpuResourceViewHeaper.hpp"

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
  friend class GraphicsCmdBuffer;

  ResourceViewManager m_heaps;
  
public:
  DescriptorHeapManager(){}

  ResourceViewManager& getGeneric()
  {
    return m_heaps;
  }
  ResourceViewManager& getSRV()
  {
    return m_heaps;
  }
  ResourceViewManager& getUAV()
  {
    return m_heaps;
  }
  ResourceViewManager& getRTV()
  {
    return m_heaps;
  }
  ResourceViewManager& getDSV()
  {
    return m_heaps;
  }

  void** getHeapPtr()
  {
    return nullptr;
  }

  unsigned getCount() // first 1 are safe to always bind to pipeline
  {
    return 1;
  }
};
