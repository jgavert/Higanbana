#include "CommandList.hpp"

bool CmdBufferBase::isValid()
{
  return m_cmdBuffer.isValid();
}

void CmdBufferBase::close()
{
  m_cmdBuffer.close();
}

bool CmdBufferBase::isClosed()
{
  return m_cmdBuffer.isClosed();
}

void DMACmdBuffer::copy(Buffer& dstdata, Buffer& srcdata)
{
  m_cmdBuffer.copy(dstdata.getBuffer(), srcdata.getBuffer());
}