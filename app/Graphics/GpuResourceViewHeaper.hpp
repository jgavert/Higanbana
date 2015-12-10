#pragma once
#include "ComPtr.hpp"
#include "core/src/entity/bitfield.hpp"
#include <d3d12.h>
#include <iostream>
#include <string>
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

  size_t getNextIndex(size_t fromBlock = 0)
  {
    // find next empty index
	size_t block = 0;
	block = m_usedIndexes.popcount_element(fromBlock);
    if (block == 128)
    {
      abort(); // lol everything used from block, cannot handle this yet.
      return 0;
    }
    else
    {
	  int emptyIndex = 0;
	  emptyIndex = m_usedIndexes.skip_find_firstEmpty(fromBlock);
      m_usedIndexes.setIdxBit(emptyIndex);
      return emptyIndex;
    }
  }

  // never called yet, should be called by resource's destructor.
  void free(size_t index)
  {
    m_usedIndexes.clearIdxBit(index);
  }

  size_t size()
  {
    return m_size;
  }
};
