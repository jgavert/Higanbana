#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"

#include <vulkan/vk_cpp.h>

class GraphicsQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  FazPtrVk<vk::Queue>    m_queue;
  GraphicsQueue(FazPtrVk<vk::Queue> queue);
public:
  void submit(GfxCommandList& list);
  void insertFence(GpuFence& fence);
  bool isValid();
  vk::Queue& get()
  {
    return m_queue.getRef();
  }
};
