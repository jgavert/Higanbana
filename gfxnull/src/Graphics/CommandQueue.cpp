#include "CommandQueue.hpp"

GpuCommandQueue::GpuCommandQueue(void* que) :m_CommandQueue(que) {}

void GpuCommandQueue::submit(GfxCommandList& )
{
}

// simple case
void GpuCommandQueue::insertFence(GpuFence& )
{
}

bool GpuCommandQueue::isValid()
{
  return true;
}
