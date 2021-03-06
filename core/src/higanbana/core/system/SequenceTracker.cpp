#include "higanbana/core/system/SequenceTracker.hpp"

#include "higanbana/core/global_debug.hpp"

namespace higanbana
{
  SequenceTracker::SequenceTracker()
    : nextSequence(1)
    , fullCompletedSeq(0)
  {
    m_incomplete.push_back(Bitfield<1>());
    m_incomplete.front().setIdxBit(0);
  }
  SeqNum SequenceTracker::next()
  {
    auto ret = nextSequence;
    nextSequence++;
    return ret;
  }
  void SequenceTracker::complete(SeqNum num)
  {
    HIGAN_ASSERT(fullCompletedSeq <= num, "Sequence was already completed %zi", num);
    auto offset = num - fullCompletedSeq;

    // first check if we have enough space to record the completed sequence
    // always done to a bitfield.
    if (offset >= static_cast<decltype(offset)>(m_incomplete.size()) * BitfieldBlockSize)
    {
      auto missingIncompletes = (offset) / BitfieldBlockSize + 1 - m_incomplete.size();
      for (decltype(missingIncompletes) i = 0; i < missingIncompletes; ++i)
      {
        m_incomplete.push_back(Bitfield<1>());
      }
    }

    // mark the bit
    auto index = offset / BitfieldBlockSize;
    auto offsetIndex = offset % BitfieldBlockSize;

    HIGAN_ASSERT(!m_incomplete[index].checkIdxBit(offsetIndex), "Sequence was already completed.");
    m_incomplete[index].setIdxBit(offsetIndex);

    // check completed full sequences
    while (m_incomplete.size() > 1)
    {
      if (m_incomplete.front().countElements() < BitfieldBlockSize)
      {
        break;
      }
      m_incomplete.pop_front();
      fullCompletedSeq += BitfieldBlockSize;
    }
  }
  bool SequenceTracker::hasCompleted(SeqNum num)
  {
    HIGAN_ASSERT(num > InvalidSeqNum, "Input wasn't valid sequence number %zi", num);
    HIGAN_ASSERT(num < nextSequence, "Too large sequence number which hasn't been started yet. %zi < %zi", num, nextSequence);
    if (num <= fullCompletedSeq)
    {
      return true;
    }
    auto offset = num - fullCompletedSeq;
    if (offset >= static_cast<decltype(offset)>(m_incomplete.size())  * BitfieldBlockSize)
    {
      return false;
    }
    auto index = offset / BitfieldBlockSize;
    auto offsetIndex = offset % BitfieldBlockSize;
    return m_incomplete[index].checkIdxBit(offsetIndex);
  }

  bool SequenceTracker::hasCompletedTill(SeqNum num)
  {
	  HIGAN_ASSERT(num > InvalidSeqNum, "Input wasn't valid sequence number %zi", num);
	  HIGAN_ASSERT(num < nextSequence, "Too large sequence number which hasn't been started yet. %zi < %zi", num, nextSequence);
	  if (num <= fullCompletedSeq)
	  {
		  return true;
	  }
	  auto offset = num - fullCompletedSeq;
	  if (offset >= static_cast<decltype(offset)>(m_incomplete.size())  * BitfieldBlockSize)
	  {
		  return false;
	  }
	  auto index = offset / BitfieldBlockSize;
	  auto offsetIndex = offset % BitfieldBlockSize;
	  for (auto i = 0; i < index; ++i)
	  {
		  if (m_incomplete[i].countElements() != BitfieldBlockSize)
		  {
			  return false;
		  }
	  }
	  for (auto k = 0; k <= offsetIndex; ++k)
	  {
		  if (!m_incomplete[index].checkIdxBit(k))
		  {
			  return false;
		  }
	  }
	  return true;
  }

  SeqNum SequenceTracker::lastSequence()
  {
    return nextSequence - 1;
  }
  
  SeqNum SequenceTracker::completedTill()
  {
    auto base = fullCompletedSeq;
    for (int i = 0; i < BitfieldBlockSize; ++i)
    {
      if (!m_incomplete[0].checkIdxBit(i))
      {
        return base + i - 1;
      }
    }
    return base + BitfieldBlockSize-1;
  }
};
