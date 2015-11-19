#pragma once
#include "ComPtr.hpp"
#include "core/src/entity/bitfield.hpp"
#include <d3d12.h>
// This is probably needed
class ResourceViewManager
{
private:
  friend class GpuDevice;
  ComPtr<ID3D12DescriptorHeap>   m_descHeap;
  size_t m_handleIncrementSize;
  size_t m_size;
  size_t index;
  faze::Bitfield<64> m_usedIndexes;

  ResourceViewManager() :
    m_descHeap(nullptr),
    m_handleIncrementSize(0),
    m_size(0),
    index(0)
  {}

  ResourceViewManager(ComPtr<ID3D12DescriptorHeap>   descHeap, size_t HandleIncrementSize, size_t size) :
    m_descHeap(descHeap),
    m_handleIncrementSize(HandleIncrementSize),
    m_size(size),
    index(0)
  {}

public:

  size_t getNextIndex()
  {
    // find next empty index
    size_t size = index;
    ++index;
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
