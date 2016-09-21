#pragma once
#include "core/src/math/vec_templated.hpp"
#include "vk_specifics/VulkanCmdBuffer.hpp"
#include "gfxvk/src/Graphics/Buffer.hpp"
#include "gfxvk/src/Graphics/Fence.hpp"
#include "core/src/system/SequenceTracker.hpp"

#include <memory>

class GraphicsCmdBuffer 
{
private:
  friend class GpuDevice;
  friend class _GpuBracket;
  friend class GpuQueue;
  CmdBufferImpl m_cmdBuffer;
  faze::SeqNum m_seqNum;
  GraphicsCmdBuffer(CmdBufferImpl cmdBuffer, faze::SeqNum num)
    : m_cmdBuffer(std::move(cmdBuffer))
    , m_seqNum(num){}
public:
  GraphicsCmdBuffer() {}
  void copy(Buffer& src, Buffer& dst);
  Fence fence();
  bool isValid();
  void close();
  bool isClosed();
};
