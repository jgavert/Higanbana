#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"
#include "FazCPtr.hpp"
#include <d3d12.h>

class GraphicsQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  FazCPtr<ID3D12CommandQueue> m_CommandQueue;
public:
  GraphicsQueue(FazCPtr<ID3D12CommandQueue> que);
  void submit(GfxCommandList& list);
  void insertFence(GpuFence& fence);
  bool isValid();
  FazCPtr<ID3D12CommandQueue> get()
  {
    return m_CommandQueue;
  }
};
