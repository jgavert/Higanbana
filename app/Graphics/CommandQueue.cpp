#include "CommandQueue.hpp"

GpuCommandQueue::GpuCommandQueue(ComPtr<ID3D12CommandQueue> que) :m_CommandQueue(que) {}

void GpuCommandQueue::submit(GfxCommandList& list)
{
  HRESULT hr;
  hr = list.m_CommandList->Close();
  if (FAILED(hr))
  {
    //
  }
  m_CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(list.m_CommandList.addr()));
}

// simple case
void GpuCommandQueue::insertFence(GpuFence& fence)
{
  auto mFence = fence.m_fence;

  mFence->Signal(0);
  //set the event to be fired once the signal value is 1
  mFence->SetEventOnCompletion(1, fence.m_handle);

  //after the command list has executed, tell the GPU to signal the fence
  m_CommandQueue->Signal(mFence.get(), 1);
}

bool GpuCommandQueue::isValid()
{
  return m_CommandQueue.get() != nullptr;
}
