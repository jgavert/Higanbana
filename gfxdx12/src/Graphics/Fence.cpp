#include "Fence.hpp"

GpuFence::GpuFence(FazCPtr<ID3D12Fence> mFence)
  :m_fence(mFence)
  , m_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr))
{
}

bool GpuFence::isSignaled()
{
  DWORD res = WaitForSingleObject(m_handle, 0);
  bool resu = false;
  switch (res)
  {
  case WAIT_ABANDONED: // wtf error!
    break;
  case WAIT_OBJECT_0: // The state of the specified object is signaled.
    resu = true;
    break;
  case WAIT_TIMEOUT: // The time - out interval elapsed, and the object's state is nonsignaled.
    break;
  case WAIT_FAILED: // The function has failed. Error error!!
    break;
  default:
    break;
  }
  return resu;
}

void GpuFence::wait()
{
  WaitForSingleObject(m_handle, INFINITE);
}
