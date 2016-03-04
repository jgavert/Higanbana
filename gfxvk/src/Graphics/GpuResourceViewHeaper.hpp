#pragma once
#include "core/src/entity/bitfield.hpp"
#include <iostream>
#include <string>
#include <assert.h>


class ResourceViewManager
{
private:
  friend class GpuDevice;
  friend class ComputeCmdBuffer;
  friend class GraphicsCmdBuffer;

  void*   m_descHeap;
  size_t m_handleIncrementSize;
  size_t m_size;

  size_t m_srvStart;
  size_t m_srvCount;
  size_t m_uavStart;
  size_t m_uavCount;
  size_t m_cbvStart;

  faze::Bitfield<8> m_usedIndexes;

  ResourceViewManager(void*   descHeap, size_t HandleIncrementSize, size_t size, size_t srvCount = 0, size_t uavCount = 0) :
	  m_descHeap(descHeap),
	  m_handleIncrementSize(HandleIncrementSize),
	  m_size(size),
	  m_srvStart(0),
	  m_srvCount(srvCount),
	  m_uavStart(srvCount),
	  m_uavCount(uavCount),
    m_cbvStart(srvCount+uavCount)
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
private:
  size_t getNextIndex(size_t fromBlock = 0)
  {
    // find next empty index
    size_t block = 0;
    block = m_usedIndexes.popcount_element(fromBlock);
    if (block == 128)
    {
      abort(); // lol everything used from block, cannot handle this yet.
    }
    else
    {
	  int emptyIndex = 0;
	  emptyIndex = static_cast<int>(m_usedIndexes.skip_find_firstEmpty(fromBlock));
      m_usedIndexes.setIdxBit(emptyIndex);
      return emptyIndex;
    }
  }

  int getNextFromRange(size_t start, size_t range)
  {
	  // find next empty index
    int64_t ret = m_usedIndexes.skip_find_firstEmpty_full_offset(start);
    if (ret == -1 || ret > static_cast<int64_t>(start + range) || ret < static_cast<int64_t>(start))
    {
      return -1;
    }
    m_usedIndexes.setIdxBit(ret);
    return static_cast<int>(ret);
  }
public:
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

  unsigned getCBVIndex()
  {
    auto val = getNextFromRange(m_cbvStart, m_size-m_cbvStart);
    if (val == -1)
    {
      abort();
    }
    return val;
  }

  unsigned getUAVStartIndex()
  {
	  return static_cast<unsigned>(m_uavStart);
  }

  int getUAVStartAddress()
  {
	  return 1;
  }

  unsigned getSRVStartIndex()
  {
	  return static_cast<unsigned>(m_srvStart);
  }

  int getSRVStartAddress()
  {
	  return 1;
  }

  unsigned getCBVStartIndex()
  {
    return static_cast<unsigned>(m_cbvStart);
  }

  int getCBVStartAddress()
  {
    return 1;
  }

  // never called yet, should be called by resource's destructor.
  /*
  void free(size_t index)
  {
    m_usedIndexes.clearIdxBit(index);
  }*/

  size_t size()
  {
    return m_size;
  }
};
