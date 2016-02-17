#include "Fence.hpp"

GpuFence::GpuFence(void* mFence)
  :m_fence(mFence)
{
}

bool GpuFence::isSignaled()
{
  return true;
}

void GpuFence::wait()
{
}
