#pragma once
#include "vk_specifics\VulkanGraphicsInstance.hpp"
#include "GpuDevice.hpp"


class GraphicsInstance
{
private:
  GraphicsInstanceImpl m_instance;

public:
  GraphicsInstance();
  bool createInstance(const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1);
  GpuDevice createGpuDevice();
};
