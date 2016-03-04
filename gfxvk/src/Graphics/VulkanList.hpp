#include "CommandList.hpp"

#include <vulkan/vk_cpp.h>

class VulkanCmdBuffer : public ICmdBuffer
{
private:
  friend class GpuDevice;
  friend class VulkanQueue;
  FazPtrVk<vk::CommandBuffer>    m_cmdBuffer;
public:
  bool isValid() = 0;
  void CopyResource() = 0;
};
