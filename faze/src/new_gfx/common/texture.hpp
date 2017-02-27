#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "implementation.hpp"

namespace faze
{
  class Texture : public backend::GpuDeviceChild
  {
    std::shared_ptr<backend::prototypes::TextureImpl> impl;
    std::shared_ptr<int64_t> id;
    ResourceDescriptor m_desc;

  public:
    Texture() = default;

    Texture(std::shared_ptr<backend::prototypes::TextureImpl> impl, std::shared_ptr<int64_t> id, ResourceDescriptor desc)
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