#pragma once
#include "FazCPtr.hpp"
#include <d3d12.h>

class GpuFence
{
private:
  friend class GpuDevice;
  friend class GraphicsQueue;
  friend class GfxCommandList;

  FazCPtr<ID3D12Fence> m_fence;
  HANDLE m_handle;
  GpuFence(FazCPtr<ID3D12Fence> mFence);
public:
  GpuFence() {}
  bool isSignaled();
  void wait();
};
