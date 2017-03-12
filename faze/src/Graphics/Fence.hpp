#pragma once

#include <inttypes.h>

// common only, no need to give real fences to user.
// checking from device and commandbuffers.
class Fence
{
private:
  int64_t m_seqNum = -1;
public:
  Fence(int64_t seqNum);
  Fence();
  int64_t get();
};
