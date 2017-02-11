#pragma once

#include "resources.hpp"

namespace faze
{
  class GpuDevice : private backend::SharedState<backend::DeviceData>
  {
  public:
    GpuDevice() = default;
    GpuDevice(backend::DeviceData data)
    {
      makeState(data);
    }
    GpuDevice(StatePtr state)
    {
      m_state = std::move(state);
    }
  };
};