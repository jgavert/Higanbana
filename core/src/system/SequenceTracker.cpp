#include "core/src/system/SequenceTracker.hpp"

#include "core/src/global_debug.hpp"

namespace faze
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
    F_ASSERT(fullCompletedSeq < num, "Sequence was already completed %zi", num);
    auto offset = num - fullCompletedSeq;

    // first check if we have enough space to record the completed sequence
    // always done to a bitfield.
    if (offset > static_cast<decltype(offset)>(m_incomplete.size()) * blockSize)
    {
      auto missingIncompletes = offset / blockSize + 1 - m_incomplete.size();
      for (auto i = 0; i < missingIncompletes; ++i)
      {
        m_incomplete.push_back(Bitfield<1>());
      }
    }

    // mark the bit
    auto index = offset / 128;
    auto offsetIndex = offset % 128;

    F_ASSERT(!m_incomplete[index].checkIdxBit(offsetIndex), "Sequence was already completed.");
    m_incomplete[index].setIdxBit(offsetIndex);

    // check completed full sequences
    while (m_incomplete.size() > 1)
    {
      if (m_incomplete.front().countElements() < blockSize)
      {
        break;
      }
      m_incomplete.pop_front();
      fullCompletedSeq += blockSize;
    }
  }
  bool SequenceTracker::hasCompleted(SeqNum num)
  {
    F_ASSERT(num > InvalidSeqNum, "Input wasn't valid sequence number %zi", num);
    F_ASSERT(num < nextSequence, "Too large sequence number which hasn't been started yet. %zi < %zi", num, nextSequence);
    if (num <= fullCompletedSeq)
    {
      return true;
    }
    auto offset = num - fullCompletedSeq;
    if (offset > static_cast<decltype(offset)>(m_incomplete.size())  * blockSize)
    {
      return false;
    }
    auto index = offset / 128;
    auto offsetIndex = offset % 128;

    return m_incomplete[index].checkIdxBit(offsetIndex);
  }
};