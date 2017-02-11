#pragma once

#include <memory>

namespace faze
{
  struct GpuDeviceData;
  class GpuDevice;

  namespace backend
  {
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
      std::weak_ptr<GpuDeviceData> m_parentDevice;
    public:
      GpuDeviceChild() = default;
      GpuDeviceChild(std::weak_ptr<GpuDeviceData> device)
        : m_parentDevice(device)
      {}

      void setParent(GpuDevice *device);
      GpuDevice device();
    };
  }
}