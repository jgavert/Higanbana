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

void CmdBufferBase::resetList()
{
  m_cmdBuffer.resetList();
}