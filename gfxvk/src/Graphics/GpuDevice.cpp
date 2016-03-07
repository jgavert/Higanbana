#include "GpuDevice.hpp"

GpuDevice::GpuDevice(GpuDeviceImpl device)
  : m_device(device)
{
}

GpuDevice::~GpuDevice()
{
}

GraphicsQueue GpuDevice::createGraphicsQueue()
{
  return m_device.createGraphicsQueue();
}

DMAQueue GpuDevice::createDMAQueue()
{
  return m_device.createDMAQueue();
}
ComputeQueue GpuDevice::createComputeQueue()
{
  return m_device.createComputeQueue();
}

DMACmdBuffer GpuDevice::createDMACommandBuffer()
{
  return m_device.createDMACommandBuffer();
}

ComputeCmdBuffer GpuDevice::createComputeCommandBuffer()
{
  return m_device.createComputeCommandBuffer();
}

GraphicsCmdBuffer GpuDevice::createGraphicsCommandBuffer()
{
  return m_device.createGraphicsCommandBuffer();
}

bool GpuDevice::isValid()
{
  return m_device.isValid();
}