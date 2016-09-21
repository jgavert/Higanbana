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

void GraphicsCmdBuffer::copy(Buffer& dstdata, Buffer& srcdata)
{
  m_cmdBuffer.copy(dstdata.getBuffer(), srcdata.getBuffer());
}

Fence GraphicsCmdBuffer::fence()
{
  return Fence(m_seqNum);
}