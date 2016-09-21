#include "VulkanCmdBuffer.hpp"

#include "core/src/global_debug.hpp"
#include "VulkanBuffer.hpp"

using namespace faze;

class BufferCopyPacket : public VulkanCommandPacket
{
private:
  std::shared_ptr<vk::Buffer> src;
  std::shared_ptr<vk::Buffer> dst;
  CommandListVector<vk::BufferCopy> m_copyList;
public:

  BufferCopyPacket(LinearAllocator& allocator, std::shared_ptr<vk::Buffer> src, std::shared_ptr<vk::Buffer> dst, MemView<vk::BufferCopy> copyList)
    : src(src)
    , dst(dst)
    , m_copyList(std::forward<decltype(copyList)>(MemView<vk::BufferCopy>(allocator.allocList<vk::BufferCopy>(copyList.size()), copyList.size())))
  {
    F_ASSERT(copyList, "Invalid copy commands");
    for (size_t i = 0; i < m_copyList.size(); i++)
    {
      m_copyList[i] = copyList[i];
    }
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    vk::ArrayProxy<const vk::BufferCopy> proxy(static_cast<uint32_t>(m_copyList.size()), m_copyList.data());
    cmd.copyBuffer(*src, *dst, proxy);
  }
};


VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer)), m_pool(std::forward<decltype(pool)>(pool)), m_closed(false)
  , m_commandList(std::make_shared<CommandList<VulkanCommandPacket>>(LinearAllocator(1024*512)))
{}

bool VulkanCmdBuffer::isValid()
{
  return m_cmdBuffer.get() != nullptr;
}
void VulkanCmdBuffer::close()
{
  m_cmdBuffer->begin(vk::CommandBufferBeginInfo()
    .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    .setPInheritanceInfo(nullptr));
  m_commandList->foreach([&](VulkanCommandPacket& packet)
  {
    packet.execute(*m_cmdBuffer);
  });
  m_cmdBuffer->end();
  m_closed = true;
}
bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}
void VulkanCmdBuffer::copy(VulkanBuffer src, VulkanBuffer dst)
{
  vk::BufferCopy copy;
  auto srcSize = src.desc().m_stride * src.desc().m_width;
  auto dstSize = dst.desc().m_stride * dst.desc().m_width;
  auto maxSize = min(srcSize, dstSize);
  copy = copy.setSize(maxSize)
    .setDstOffset(0)
    .setSrcOffset(0);
  m_commandList->insert<BufferCopyPacket>(src.m_resource, dst.m_resource, copy);
}
