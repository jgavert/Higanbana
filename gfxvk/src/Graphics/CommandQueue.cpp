#include "CommandQueue.hpp"

GraphicsQueue::GraphicsQueue(FazPtrVk<vk::Queue> queue) :m_queue(queue) {}

void GraphicsQueue::submit(GfxCommandList& )
{
}

// simple case
void GraphicsQueue::insertFence(GpuFence& )
{
}

bool GraphicsQueue::isValid()
{
  return true;
}
