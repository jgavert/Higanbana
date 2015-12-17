#pragma once
#include "ComPtr.hpp"
#include "core/src/entity/bitfield.hpp"
#include <d3d12.h>
#include <iostream>
#include <string>
#include <assert.h>


class ResourceViewManager
{
private:
  friend class GpuDevice;
  friend class CptCommandList;
  friend class GfxCommandList;

  ComPtr<ID3D12DescriptorHeap>   m_descHeap;
  size_t m_handleIncrementSize;
  size_t m_size;

  size_t m_srvStart;
  size_t m_srvCount;
  size_t m_uavStart;
  size_t m_uavCount;
  size_t m_restStart;

  faze::Bitfield<8> m_usedIndexes;

  ResourceViewManager(ComPtr<ID3D12DescriptorHeap>   descHeap, size_t HandleIncrementSize, size_t size, size_t srvCount = 0, size_t uavCount = 0) :
	  m_descHeap(descHeap),
	  m_handleIncrementSize(HandleIncrementSize),
	  m_size(size),
	  m_srvStart(0),
	  m_uavStart(srvCount),
	  m_srvCount(srvCount),
	  m_uavCount(uavCount)
  {
    assert(size >= srvCount + uavCount && size < 128*8);
  }


public:
	ResourceViewManager() :
		m_descHeap(nullptr),
		m_handleIncrementSize(0),
		m_size(0),
		m_srvStart(0),
		m_srvCount(0),
		m_uavStart(0),
		m_uavCount(0)
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
	  emptyIndex = static_cast<int>(m_usedIndexes.skip_find_firstEmpty(fromBlock));
      m_usedIndexes.setIdxBit(emptyIndex);
      return emptyIndex;
    }
  }

  // this needs heavy testing to see if it works as expected
  int getNextFromRange(size_t start, size_t range)
  {
	  // find next empty index
	  size_t block = start / 128;
	  size_t offset = start % 128;
	  size_t ret = 0;
	  ret = m_usedIndexes.popcount_element(block);
	  if (ret == 128)
	  {
		  return -1;
	  }
	  else
	  {
		  int emptyIndex = 0;
		  emptyIndex = static_cast<int>(m_usedIndexes.skip_find_firstEmpty_offset(block, offset));
		  if (emptyIndex <= offset+range && emptyIndex >= offset)
		  {
			m_usedIndexes.setIdxBit(128*block + emptyIndex);
			return 128*block + emptyIndex;
		  }
		  else
		  {
			return -1;
		  }
	  }
  }

  unsigned getUAVIndex()
  {
	  auto val = getNextFromRange(m_uavStart, m_uavCount);
	  if (val == -1)
	  {
		  abort();
	  }
	  return val;
  }
  
  unsigned getSRVIndex()
  {
	  auto val = getNextFromRange(m_srvStart, m_srvCount);
	  if (val == -1)
	  {
		  abort();
	  }
	  return val;
  }

  unsigned getUAVStartIndex()
  {
	  return m_uavStart;
  }

  D3D12_GPU_DESCRIPTOR_HANDLE getUAVStartAddress()
  {
	  D3D12_GPU_DESCRIPTOR_HANDLE woot = m_descHeap->GetGPUDescriptorHandleForHeapStart();
	  woot.ptr += m_uavStart*m_handleIncrementSize;
	  return woot;
  }

  unsigned getSRVStartIndex()
  {
	  return m_srvStart;
  }

  D3D12_GPU_DESCRIPTOR_HANDLE getSRVStartAddress()
  {
	  D3D12_GPU_DESCRIPTOR_HANDLE woot = m_descHeap->GetGPUDescriptorHandleForHeapStart();
	  woot.ptr += m_srvStart*m_handleIncrementSize;
	  return woot;
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
