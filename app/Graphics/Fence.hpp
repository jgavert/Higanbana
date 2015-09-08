#pragma once
#include "ComPtr.hpp"
#include <d3d12.h>

class GpuFence
{
private:
  friend class GpuDevice;
  friend class GpuCommandQueue;
  friend class GfxCommandList;

  ComPtr<ID3D12Fence> m_fence;
  HANDLE m_handle;
  GpuFence(ComPtr<ID3D12Fence> mFence);
public:
  GpuFence() {}
  bool isSignaled();
  void wait();
};
