#include "CommandQueue.hpp"

GraphicsQueue::GraphicsQueue(void* que) :m_CommandQueue(que) {}

void GraphicsQueue::submit(GraphicsCmdBuffer& )
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
