#pragma once

#include "higanbana/core/entity/bitfield.hpp"

#include <deque>

namespace higanbana
{
  using SeqNum = int64_t;
  static constexpr SeqNum InvalidSeqNum = -1;
  class SequenceTracker
  {
    constexpr static int64_t BitfieldBlockSize = 128;
    SeqNum nextSequence = InvalidSeqNum;
    SeqNum fullCompletedSeq = InvalidSeqNum;
    std::deque<Bitfield<1>> m_incomplete;

  public:
    SequenceTracker();
    SeqNum next();
    void complete(SeqNum num);
    bool hasCompleted(SeqNum num);
	  bool hasCompletedTill(SeqNum num);
    SeqNum completedTill();
	  SeqNum lastSequence();
  };
}
