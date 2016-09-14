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
  CmdBufferBase(CmdBufferImpl cmdBuffer) : m_cmdBuffer(std::move(cmdBuffer)) {}
public:
  bool isValid();
  void close();
  bool isClosed();
  void resetList();
};

class DMACmdBuffer : public CmdBufferBase
{
private:
  friend class GpuDevice;
  friend class ComputeCmdBuffer;
  DMACmdBuffer(CmdBufferImpl cmdBuffer) : CmdBufferBase(std::move(cmdBuffer)) {}
public:
  //void CopyResource(Buffer& dstdata, Buffer& srcdata); // this is here only temporarily, will be removed
  // copy stuff
};

class ComputeCmdBuffer : public DMACmdBuffer
{
private:
  friend class GpuQueue;
  friend class GpuDevice;
  friend class GraphicsCmdBuffer;

  ComputeCmdBuffer(CmdBufferImpl cmdBuffer) : DMACmdBuffer(std::move(cmdBuffer)) {}
public:
  // compute stuff
};

class GraphicsCmdBuffer : public ComputeCmdBuffer
{
private:
  friend class GpuQueue;
  friend class GpuDevice;
  GraphicsCmdBuffer(CmdBufferImpl cmdList) : ComputeCmdBuffer(std::move(cmdList)){}
public:
  // graphics stuff
};
