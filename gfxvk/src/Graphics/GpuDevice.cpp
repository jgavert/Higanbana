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
  auto ret = m_device.createGraphicsQueue();
  return GraphicsQueue(ret);
}

GraphicsCmdBuffer GpuDevice::createGraphicsCommandBuffer()
{
  auto ret = m_device.createGraphicsCommandBuffer();
  return GraphicsCmdBuffer(ret);
}

bool GpuDevice::isValid()
{
  return m_device.isValid();
}