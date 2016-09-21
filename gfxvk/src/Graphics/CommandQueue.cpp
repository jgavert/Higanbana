#include "CommandQueue.hpp"

GpuQueue::GpuQueue(QueueImpl queue) :m_queue(queue)
{

}
// simple case
void GpuQueue::insertFence(GpuFence& )
{
}

bool GpuQueue::isValid()
{
  return m_queue.isValid();
}

DMAQueue::DMAQueue(QueueImpl queue) : GpuQueue(queue) {};

void DMAQueue::submit(DMACmdBuffer& dma)
{
  dma.close();
}

ComputeQueue::ComputeQueue(QueueImpl queue) : GpuQueue(queue) {};

void ComputeQueue::submit(ComputeCmdBuffer& )
{

}

GraphicsQueue::GraphicsQueue(QueueImpl queue) : ComputeQueue(queue) {}

void GraphicsQueue::submit(GraphicsCmdBuffer& )
{

}


