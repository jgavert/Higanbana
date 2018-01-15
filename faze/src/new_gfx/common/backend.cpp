#include "gpudevice.hpp"

namespace faze
{
  namespace backend
  {
    void GpuDeviceChild::setParent(GpuDevice *device)
    {
      m_parentDevice = device->state();
    }
    GpuDevice GpuDeviceChild::device()
    {
      return GpuDevice(m_parentDevice.lock());
    }
  }
}