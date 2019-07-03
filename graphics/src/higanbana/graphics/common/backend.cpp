#include "higanbana/graphics/common/gpu_group.hpp"

namespace higanbana
{
  namespace backend
  {
    void GpuGroupChild::setParent(GpuGroup *device)
    {
      m_parentDevice = device->state();
    }
    GpuGroup GpuGroupChild::device()
    {
      return GpuGroup(m_parentDevice.lock());
    }
  }
}