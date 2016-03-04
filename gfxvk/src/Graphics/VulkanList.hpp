#include "CommandList.hpp"

#include <vulkan/vk_cpp.h>

class VulkanCmdBuffer
{
private:
  friend class GpuDevice;
  friend class VulkanQueue;
  FazPtrVk<vk::CommandBuffer>    m_cmdBuffer;
public:
  bool isValid();
  void CopyResource();
};
