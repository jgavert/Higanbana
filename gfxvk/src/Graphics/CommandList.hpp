#pragma once
#include "core/src/math/vec_templated.hpp"
#include "vk_specifics/VulkanCmdBuffer.hpp"

#include <memory>

class CmdBufferBase
{
private:
  friend class GpuDevice;
  friend class DMACmdBuffer;
  friend class ComputeCmdBuffer;
  friend class GraphicsCmdBuffer;
  friend class _GpuBracket;
  friend class GpuQueue;
  CmdBufferImpl m_cmdBuffer;
  CmdBufferBase(CmdBufferImpl cmdBuffer) : m_cmdBuffer(cmdBuffer) {}
public:
  bool isValid();
  void closeList();
  bool isClosed();
  void resetList();
};

class DMACmdBuffer : public CmdBufferBase
{
private:
  friend class GpuDevice;
  DMACmdBuffer(CmdBufferImpl cmdBuffer) : CmdBufferBase(cmdBuffer) {}
public:
  //void CopyResource(Buffer& dstdata, Buffer& srcdata); // this is here only temporarily, will be removed
  // copy stuff
};



class ComputeCmdBuffer : public CmdBufferBase
{
private:
  friend class GpuQueue;
  friend class GpuDevice;
  friend class GraphicsCmdBuffer;

  ComputeCmdBuffer(CmdBufferImpl cmdBuffer) : CmdBufferBase(cmdBuffer) {}
public:
  // compute stuff
};

class GraphicsCmdBuffer : public ComputeCmdBuffer
{
private:
  friend class GpuQueue;
  friend class GpuDevice;
  GraphicsCmdBuffer(CmdBufferImpl cmdList) :ComputeCmdBuffer(cmdList){}
public:
  // graphics stuff
};
