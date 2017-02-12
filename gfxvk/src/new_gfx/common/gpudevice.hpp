#pragma once

#include "resources.hpp"

#include "resource_descriptor.hpp"

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

    StatePtr state()
    {
      return m_state;
    }
    void createBuffer(ResourceDescriptor descriptor)
    {
      //S().impl->createBuffer(descriptor)
    }
  };
};