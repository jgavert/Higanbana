#pragma once
#include "CommandQueue.hpp"

#include <vulkan/vk_cpp.h>


class VulkanQueue : public IQueue
{
private:
  friend class GpuDevice;
  FazPtrVk<vk::Queue>    m_queue;
public:
  bool isValid() override
  {
    return m_queue.isValid();
  }
  void insertFence() override
  {
  }
  void submit(ICmdBuffer& buffer) override
  {
  }
};
