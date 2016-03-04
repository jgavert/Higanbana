#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"

class GraphicsQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  void* m_CommandQueue;
public:
  GraphicsQueue(void* que);
  void submit(GraphicsCmdBuffer& list);
  void insertFence(GpuFence& fence);
  bool isValid();
  void* get()
  {
    return m_CommandQueue;
  }
};
