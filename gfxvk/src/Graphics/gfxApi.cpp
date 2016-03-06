#include "gfxApi.hpp"

GraphicsInstance::GraphicsInstance()
{

}

bool GraphicsInstance::createInstance(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
{
  return m_instance.createInstance(appName, appVersion, engineName, engineVersion);
}

GpuDevice GraphicsInstance::createGpuDevice()
{
  return GpuDevice(m_instance.createGpuDevice());
}