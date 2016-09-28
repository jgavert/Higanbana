#pragma once
#include "core/src/math/vec_templated.hpp"
#include "vk_specifics/VulkanCmdBuffer.hpp"
#include "vk_specifics/VulkanGpuDevice.hpp"
#include "gfxvk/src/Graphics/Buffer.hpp"
#include "gfxvk/src/Graphics/Fence.hpp"
#include "gfxvk/src/Graphics/Pipeline.hpp"
#include "gfxvk/src/Graphics/DescriptorPool.hpp"
#include "core/src/system/SequenceTracker.hpp"
#include "DescriptorSet.hpp"

#include <memory>

class GraphicsCmdBuffer 
{
private:
  friend class GpuDevice;
  friend class _GpuBracket;
  friend class GpuQueue;
  CmdBufferImpl m_cmdBuffer;
  std::shared_ptr<GpuDeviceImpl> m_device;
  faze::SeqNum m_seqNum;
  DescriptorPool m_pool;
  GraphicsCmdBuffer(std::shared_ptr<GpuDeviceImpl>& device, CmdBufferImpl cmdBuffer, faze::SeqNum num, DescriptorPool pool)
    : m_cmdBuffer(std::move(cmdBuffer))
    , m_device(device)
    , m_seqNum(num)
    , m_pool(pool)
  {}

  void doDescriptorSets(DescriptorSet& inputs);
public:
  GraphicsCmdBuffer() {}
  void copy(Buffer& src, Buffer& dst);
  void bindPipeline(ComputePipeline& pipeline);
  //void bindPipeline(GraphicsPipeline& pipeline);
  void dispatch(DescriptorSet& inputs, unsigned x, unsigned y, unsigned z);
  Fence fence();
  bool isValid();
  void close();
  bool isClosed();

  // shader

  template <typename ShaderInterface>
  DescriptorSet bind(ComputePipeline& pipeline)
  {
    m_cmdBuffer.bindComputePipeline(pipeline.m_pipeline);
	auto set = m_device->allocateDescriptorSet(m_pool.impl(), pipeline.impl());
	return DescriptorSet(set);
  }
};
