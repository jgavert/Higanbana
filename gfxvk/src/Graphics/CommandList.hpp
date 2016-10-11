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
  std::shared_ptr<CmdBufferImpl> m_cmdBuffer;
  std::shared_ptr<GpuDeviceImpl> m_device;
  faze::SeqNum m_seqNum;
  DescriptorPool m_pool;
  GraphicsCmdBuffer(std::shared_ptr<GpuDeviceImpl>& device, std::shared_ptr<CmdBufferImpl> cmdBuffer, faze::SeqNum num, DescriptorPool pool)
    : m_cmdBuffer(cmdBuffer)
    , m_device(device)
    , m_seqNum(num)
    , m_pool(pool)
  {}

public:
  GraphicsCmdBuffer() {}
  void copy(Buffer& src, Buffer& dst);
  void clearRTV(TextureRTV& texture, float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.0f);
  void bindPipeline(ComputePipeline& pipeline);
  //void bindPipeline(GraphicsPipeline& pipeline);
  void dispatch(DescriptorSet& inputs, unsigned x, unsigned y = 1, unsigned z = 1);
  void dispatchThreads(DescriptorSet& inputs, unsigned x, unsigned y = 1, unsigned z = 1);
  Fence fence();
  bool isValid();
  void close();
  bool isClosed();
  void prepareForSubmit(GpuDeviceImpl& device);

  void prepareForPresent(Texture& texture);
  // shader

  template <typename ShaderInterface>
  DescriptorSet bind(ComputePipeline& pipeline)
  {
    m_cmdBuffer->bindComputePipeline(pipeline.m_pipeline);
    auto descriptorImpl = DescriptorSetImpl(pipeline.impl());
    auto workGroupX = ShaderInterface::s_workGroupSizeX;
    auto workGroupY = ShaderInterface::s_workGroupSizeY;
    auto workGroupZ = ShaderInterface::s_workGroupSizeZ;
	  return DescriptorSet(descriptorImpl, workGroupX, workGroupY, workGroupZ);
  }
};
