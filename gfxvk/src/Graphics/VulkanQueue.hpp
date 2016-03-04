#pragma once
#include "VulkanList.hpp"
#include <vulkan/vk_cpp.h>


class VulkanQueue
{
private:
  friend class GpuDevice;
  FazPtrVk<vk::Queue>    m_queue;
public:
  bool isValid()
  {
    return m_queue.isValid();
  }
  void insertFence()
  {
  }
  void submit(VulkanCmdBuffer& buffer)
  {
  }
};
