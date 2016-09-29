#include "CommandList.hpp"


bool GraphicsCmdBuffer::isValid()
{
  return m_cmdBuffer.isValid();
}

void GraphicsCmdBuffer::close()
{
  m_cmdBuffer.close();
}

bool GraphicsCmdBuffer::isClosed()
{
  return m_cmdBuffer.isClosed();
}

Fence GraphicsCmdBuffer::fence()
{
  return Fence(m_seqNum);
}

// binding
void GraphicsCmdBuffer::bindPipeline(ComputePipeline& pipeline)
{
  m_cmdBuffer.bindComputePipeline(pipeline.m_pipeline);
}
// compute

void GraphicsCmdBuffer::dispatch(DescriptorSet& inputs, unsigned x, unsigned y, unsigned z)
{
  doDescriptorSets(inputs);
  m_cmdBuffer.bindComputeDescriptorSet(inputs.set);
  m_cmdBuffer.dispatch(x, y, z);
}

// draw

// copy

void GraphicsCmdBuffer::copy(Buffer& srcdata, Buffer& dstdata)
{
  m_cmdBuffer.copy(srcdata.getBuffer(), dstdata.getBuffer());
}

void GraphicsCmdBuffer::doDescriptorSets(DescriptorSet& inputs)
{
	m_device->writeDescriptorSet(inputs.set);
}