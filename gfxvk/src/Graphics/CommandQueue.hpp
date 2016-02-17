#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"

class GpuCommandQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  void* m_CommandQueue;
public:
  GpuCommandQueue(void* que);
  void submit(GfxCommandList& list);
  void insertFence(GpuFence& fence);
  bool isValid();
  void* get()
  {
    return m_CommandQueue;
  }
};
