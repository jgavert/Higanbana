#include "gfxApi.hpp"

GraphicsInstance::GraphicsInstance()
{

}

bool GraphicsInstance::createInstance(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
{
  return m_instance.createInstance(appName, appVersion, engineName, engineVersion);
}

GpuDevice GraphicsInstance::createGpuDevice(FileSystem& fs)
{
  return GpuDevice(m_instance.createGpuDevice(fs));
}

WindowSurface GraphicsInstance::createSurface(Window& window)
{
	WindowSurface surface;
	surface.impl = m_instance.createSurface(window.getInternalWindow().getHWND(), window.getInternalWindow().getHInstance());
	return surface;
}