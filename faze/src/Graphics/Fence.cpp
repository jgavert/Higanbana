#include "Fence.hpp"

Fence::Fence()
{
}
Fence::Fence(int64_t seqNum)
  :m_seqNum(seqNum)
{
}

int64_t Fence::get()
{
  return m_seqNum;
}