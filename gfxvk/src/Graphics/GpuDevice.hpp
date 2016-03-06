#pragma once
#include "vk_specifics/VulkanGpuDevice.hpp"
#include "CommandList.hpp"
#include "CommandQueue.hpp"

class GpuDevice
{
private:
  friend class GraphicsInstance;
  GpuDeviceImpl m_device;
  GpuDevice(GpuDeviceImpl device);
public:
  ~GpuDevice();
  GraphicsQueue createGraphicsQueue();
  GraphicsCmdBuffer createGraphicsCommandBuffer();
  bool isValid();
};
