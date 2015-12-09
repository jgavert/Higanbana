#pragma once
#include "ComPtr.hpp"
#include "core/src/entity/bitfield.hpp"
#include <d3d12.h>
// This is probably needed
class ResourceViewManager
{
private:
  friend class GpuDevice;
  friend class CptCommandList;
  friend class GfxCommandList;

  ComPtr<ID3D12DescriptorHeap>   m_descHeap;
  size_t m_handleIncrementSize;
  size_t m_size;
  size_t m_index;

  faze::Bitfield<8> m_usedIndexes;

  ResourceViewManager(ComPtr<ID3D12DescriptorHeap>   descHeap, size_t HandleIncrementSize, size_t size) :
    m_descHeap(descHeap),
    m_handleIncrementSize(HandleIncrementSize),
    m_size(size),
    m_index(0)
  {
    assert(size < 8 * 128);
  }


public:
  ResourceViewManager() :
    m_descHeap(nullptr),
    m_handleIncrementSize(0),
    m_size(0),
    m_index(0)
  {}

  size_t getNextIndex()
  {
    // find next empty index
    size_t size = m_index;
    ++m_index;
    return size;
  }

  void free(size_t index)
  {
    m_usedIndexes.clearIdxBit(index);
  }

  size_t size()
  {
    return m_size;
  }
};
