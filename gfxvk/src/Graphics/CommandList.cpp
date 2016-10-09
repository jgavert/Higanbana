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

// Pipeline
void GraphicsCmdBuffer::bindPipeline(ComputePipeline& pipeline)
{
  m_cmdBuffer.bindComputePipeline(pipeline.m_pipeline);
}
// compute

void GraphicsCmdBuffer::dispatch(DescriptorSet& inputs, unsigned x, unsigned y, unsigned z)
{
  m_cmdBuffer.dispatch(inputs.set, x, y, z);
}

void GraphicsCmdBuffer::dispatchThreads(DescriptorSet& inputs, unsigned x, unsigned y, unsigned z)
{
  const auto roundUpMultiple = [](int value, int multiple)
  {
    return (value + multiple - 1) / multiple * multiple;
  };
  auto xThreads = roundUpMultiple(x, inputs.workGroupX);
  auto yThreads = roundUpMultiple(y, inputs.workGroupY);
  auto zThreads = roundUpMultiple(z, inputs.workGroupZ);
  m_cmdBuffer.dispatch(inputs.set, xThreads, yThreads, zThreads);
}

// draw

// copy

void GraphicsCmdBuffer::copy(Buffer& srcdata, Buffer& dstdata)
{
  m_cmdBuffer.copy(srcdata.getBuffer(), dstdata.getBuffer());
}

void GraphicsCmdBuffer::prepareForSubmit(GpuDeviceImpl& device)
{
  m_cmdBuffer.prepareForSubmit(device, m_pool.impl());
}