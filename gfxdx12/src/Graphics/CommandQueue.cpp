#include "CommandQueue.hpp"

GraphicsQueue::GraphicsQueue(FazCPtr<ID3D12CommandQueue> que) :m_CommandQueue(que) {}

void GraphicsQueue::submit(GraphicsCmdBuffer& list)
{
  if (!list.isClosed())
  {
    abort();
  }
  m_CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(list.m_CommandList.addr()));
}

// simple case
void GraphicsQueue::insertFence(GpuFence& fence)
{
  auto mFence = fence.m_fence;

  mFence->Signal(0);
  //set the event to be fired once the signal value is 1
  mFence->SetEventOnCompletion(1, fence.m_handle);

  //after the command list has executed, tell the GPU to signal the fence
  m_CommandQueue->Signal(mFence.get(), 1);
}

bool GraphicsQueue::isValid()
{
  return m_CommandQueue.get() != nullptr;
}
