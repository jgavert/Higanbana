#pragma once
#include "higanbana/core/system/SequenceTracker.hpp"

#include <deque>
#include <inttypes.h>


namespace higanbana
{

  class ValueRange
  {
  private:
    int64_t offset = 0;
    int64_t size = 0;
  public:
    ValueRange();
    ValueRange(int64_t start, int64_t size);
    int64_t start();
    int64_t end();
    int64_t count();
  };

  class SequenceRingRangeAllocator 
  {
  public:
    SequenceRingRangeAllocator();
    SequenceRingRangeAllocator(int64_t size);

    ValueRange nextRange(SeqNum seqNum, int64_t size);

	  template <typename Func>
	  void sequenceCompleted(Func isCompleted)
	  {
		  while (!reservedData.empty())
		  {
			  auto& usedRange = reservedData.front();
			  if (!isCompleted(usedRange.sequence))
				  break;
			  if (usedBegin > usedRange.end)
			  {
				  usedEnd = currentPosition;
			  }
			  usedBegin = usedRange.end;
        reservedData.pop_front();
		  }
	  }

    int64_t rangeSize();

  private:
	  int64_t rangeLength = 0;

	  struct ReservedRange
	  {
		  SeqNum sequence = 0;
		  int64_t end = 0;
	  };
	  std::deque<ReservedRange> reservedData;

	  int64_t currentPosition = 0;
	  int64_t usedBegin = 0;
	  int64_t usedEnd = 0;

	  int64_t availableSpace();
	  bool ensureSpaceNeeded(int64_t size);
	  void registerUsedRange(SeqNum num, int64_t size);
  };
}
