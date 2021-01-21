#pragma once

#include <memory>
namespace higanbana
{
  class GpuGroup;
  namespace backend
  {
    struct DeviceGroupData;
    class GpuGroupChild
    {
      std::weak_ptr<DeviceGroupData> m_parentDevice;
    public:
      GpuGroupChild() = default;
      GpuGroupChild(std::weak_ptr<DeviceGroupData> device)
        : m_parentDevice(device)
      {}

      void setParent(GpuGroup *device);
      GpuGroup device();
    };
  }
}