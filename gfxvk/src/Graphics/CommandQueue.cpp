#include "CommandQueue.hpp"

Queue_::Queue_(PQueue queue) :m_queue(queue)
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

DMAQueue::DMAQueue(PQueue queue) : Queue_(queue) {};

void DMAQueue::submit(DMACmdBuffer& )
{

}

ComputeQueue::ComputeQueue(PQueue queue) : Queue_(queue) {};

void ComputeQueue::submit(ComputeCmdBuffer& )
{

}

GraphicsQueue::GraphicsQueue(PQueue queue) : ComputeQueue(queue) {}

void GraphicsQueue::submit(GraphicsCmdBuffer& )
{

}


