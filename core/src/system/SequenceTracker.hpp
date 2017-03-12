#pragma once

#include "core/src/entity/bitfield.hpp"

#include <deque>

namespace faze
{
  using SeqNum = int64_t;
  static constexpr SeqNum InvalidSeqNum = -1;
  class SequenceTracker
  {
    static constexpr int64_t blockSize = 128;
    SeqNum nextSequence = InvalidSeqNum;
    SeqNum fullCompletedSeq = InvalidSeqNum;
    std::deque<Bitfield<1>> m_incomplete;

  public:
    SequenceTracker();
    SeqNum next();
    void complete(SeqNum num);
    bool hasCompleted(SeqNum num);
    SeqNum lastSequence();
  };
}
