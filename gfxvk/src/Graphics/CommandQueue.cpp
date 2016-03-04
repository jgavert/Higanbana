#include "CommandQueue.hpp"

Queue_::Queue_(std::shared_ptr<IQueue> queue) :m_queue(queue)
{

}
// simple case
void Queue_::insertFence(GpuFence& )
{
}

bool Queue_::isValid()
{
  return m_queue->isValid();
}

DMAQueue::DMAQueue(std::shared_ptr<IQueue> queue) : Queue_(queue) {};

void DMAQueue::submit(DMACmdBuffer& )
{

}

ComputeQueue::ComputeQueue(std::shared_ptr<IQueue> queue) : Queue_(queue) {};

void ComputeQueue::submit(ComputeCmdBuffer& )
{

}

GraphicsQueue::GraphicsQueue(std::shared_ptr<IQueue> queue) : ComputeQueue(queue) {}

void GraphicsQueue::submit(GraphicsCmdBuffer& )
{

}


