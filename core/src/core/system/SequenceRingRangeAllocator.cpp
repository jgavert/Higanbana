#include "core/system/SequenceRingRangeAllocator.hpp"

#include <deque>
#include <inttypes.h>


namespace faze
{
    ValueRange::ValueRange() {}
    ValueRange::ValueRange(int64_t start, int64_t size)
      : offset(start)
      , size(size)
    {}
    int64_t  ValueRange::start()
    {
      return offset;
    }
    int64_t  ValueRange::end()
    {
      return offset + size;
    }
    int64_t  ValueRange::count()
    {
      return size;
    }

    SequenceRingRangeAllocator::SequenceRingRangeAllocator()
    {}
    SequenceRingRangeAllocator::SequenceRingRangeAllocator(int64_t size)
      :rangeLength(size)
    {}

	  ValueRange SequenceRingRangeAllocator::nextRange(SeqNum seqNum, int64_t size)
	  {
		  if (size == 0 || !ensureSpaceNeeded(size))
			  return ValueRange();
		  size_t pos = currentPosition;
		  registerUsedRange(seqNum, size);
		  return ValueRange(pos, size);
	  }

    int64_t SequenceRingRangeAllocator::rangeSize()
    {
      return rangeLength;
    }

	  int64_t SequenceRingRangeAllocator::availableSpace()
	  {
		  if (usedEnd <= currentPosition)
		  {
			  return rangeLength - currentPosition;
		  }
		  return usedBegin - currentPosition;
	  }

	  bool SequenceRingRangeAllocator::ensureSpaceNeeded(int64_t size)
	  {
		  if (usedBegin <= currentPosition)
		  {
			  if (currentPosition + size > rangeLength)
			  {
				  if (usedBegin != 0)
				  {
					  currentPosition = 0;
					  if (!reservedData.empty())
					  {
              reservedData.back().end = 0;
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

	  void SequenceRingRangeAllocator::registerUsedRange(SeqNum num, int64_t size)
	  {
		  currentPosition += size;
		  if (!reservedData.empty() && reservedData.back().sequence == num)
		  {
        reservedData.back().end = currentPosition;
		  }
		  else
		  {
			  ReservedRange range;
			  range.end = currentPosition;
			  range.sequence = num;
        reservedData.push_back(range);
		  }
		  usedEnd += size;
	  }
}
