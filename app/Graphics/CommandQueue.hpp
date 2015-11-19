#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"
#include "ComPtr.hpp"
#include <d3d12.h>

class GpuCommandQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  ComPtr<ID3D12CommandQueue> m_CommandQueue;
public:
  GpuCommandQueue(ComPtr<ID3D12CommandQueue> que);
  void submit(GfxCommandList& list);
  void insertFence(GpuFence& fence);
  bool isValid();
  ComPtr<ID3D12CommandQueue> get()
  {
    return m_CommandQueue;
  }
};
