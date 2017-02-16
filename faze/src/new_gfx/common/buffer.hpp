#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"

namespace faze
{
  class Buffer : private backend::SharedState<backend::BufferData>, public backend::GpuDeviceChild
  {
  public:
    Buffer() = default;
    Buffer(backend::BufferData data)
    {
      makeState(data);
    }

    ResourceDescriptor& desc()
    {
      return S().desc;
    }

    StatePtr state()
    {
      return m_state;
    }
  };
};