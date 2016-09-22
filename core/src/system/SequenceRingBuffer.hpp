#pragma once
#include "core/src/system/SequenceTracker.hpp"

#include <deque>
#include <inttypes.h>


namespace faze
{
  class SequenceRingBuffer 
  {
  public:

	  void nextRange(SeqNum seqNum, int64_t size)
	  {
		  if (size == 0 || !ensureSpaceNeeded(size))
			  return;
		  size_t pos = currentPosition;
		  registerUsedRange(seqNum, size);
		  return MemView<uint8_t>(m_data.data() + pos, size);
	  }

	  template <typename Func>
	  void sequenceCompleted(Func isCompleted)
	  {
		  while (!m_reservedData.empty())
		  {
			  auto& usedRange = m_reservedData.front();
			  if (!isCompleted(usedRange.sequence))
				  break;
			  if (usedBegin > usedRange.end)
			  {
				  usedEnd = currentPosition;
			  }
			  usedBegin = usedRange.end;
			  usedEnd.pop_front();
		  }
	  }

  private:
	  int64_t rangeLength;

	  struct ReservedRange
	  {
		  SeqNum sequence = 0;
		  int64_t end = 0;
	  };
	  std::deque<ReservedRange> m_reservedData;

	  int64_t currentPosition = 0;
	  int64_t usedBegin = 0;
	  int64_t usedEnd = 0;

	  int64_t availableSpace()
	  {
		  if (usedEnd <= currentPosition)
		  {
			  return rangeLength - currentPosition;
		  }
		  return usedBegin - currentPosition;
	  }

	  bool ensureSpaceNeeded(int64_t size)
	  {
		  if (usedBegin <= currentPosition)
		  {
			  if (currentPosition + size > rangeLength)
			  {
				  if (usedBegin != 0)
				  {
					  currentPosition = 0;
					  if (!m_reservedData.empty())
					  {
						  m_reservedData.back().end = 0;
					  }
					  else
					  {
						  usedBegin = 0;
						  usedEnd = currentPosition;
					  }
				  }
			  }
		  }
		  if (availableSpace() >= size)
			  return true;
		  return false;
	  }

	  void registerUsedRange(SeqNum num, int64_t size)
	  {
		  currentPosition += size;
		  if (!m_reservedData.empty() && m_reservedData.back().sequence == num)
		  {
			  m_reservedData.back().end = currentPosition;
		  }
		  else
		  {
			  ReservedRange range;
			  range.end = currentPosition;
			  range.sequence = num;
			  m_reservedData.push_back(range);
		  }
		  usedEnd += size;
	  }
  };
}
