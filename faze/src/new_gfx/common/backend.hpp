#pragma once

#include "resource_descriptor.hpp"
#include <memory>

namespace faze
{
  class GpuDevice;
  namespace backend
  {
    struct DeviceData;

    template <typename T>
    class SharedState
    {
    protected:
      using State = T;
      using StatePtr = std::shared_ptr<T>;

      StatePtr m_state;
    public:

      template <typename... Ts>
      T &makeState(Ts &&... ts)
      {
        m_state = std::make_shared<T>(std::forward<Ts>(ts)...);
        return S();
      }

      const T &S() const { return *m_state; }
      T &S() { return *m_state; }

      bool valid() const { return !!m_state; }
    };

    class GpuDeviceChild
    {
      std::weak_ptr<DeviceData> m_parentDevice;
    public:
      GpuDeviceChild() = default;
      GpuDeviceChild(std::weak_ptr<DeviceData> device)
        : m_parentDevice(device)
      {}

      void setParent(GpuDevice *device);
      GpuDevice device();
    };
  }
}