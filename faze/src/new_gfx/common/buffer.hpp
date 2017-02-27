#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "implementation.hpp"

namespace faze
{
  class Buffer : public backend::GpuDeviceChild
  {
    std::shared_ptr<backend::prototypes::BufferImpl> impl;
    std::shared_ptr<int64_t> id;
    ResourceDescriptor m_desc;

  public:
    Buffer() = default;

    Buffer(std::shared_ptr<backend::prototypes::BufferImpl> impl, std::shared_ptr<int64_t> id, ResourceDescriptor desc)
      : impl(impl)
      , id(id)
      , m_desc(std::move(desc))
    {
    }

    ResourceDescriptor& desc()
    {
      return m_desc;
    }
  };
};