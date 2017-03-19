#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "prototypes.hpp"

namespace faze
{
  class Buffer : public backend::GpuDeviceChild
  {
    std::shared_ptr<backend::prototypes::BufferImpl> impl;
    std::shared_ptr<int64_t> id;
    std::shared_ptr<ResourceDescriptor> m_desc;

  public:
    Buffer()
      : m_desc(std::make_shared<ResourceDescriptor>())
    {
    }
    Buffer(std::shared_ptr<backend::prototypes::BufferImpl> impl, std::shared_ptr<int64_t> id, ResourceDescriptor desc)
      : impl(impl)
      , id(id)
      , m_desc(std::make_shared<ResourceDescriptor>(std::move(desc)))
    {
    }

    ResourceDescriptor& desc()
    {
      return *m_desc;
    }

    std::shared_ptr<backend::prototypes::BufferImpl> native()
    {
      return impl;
    }
  };

  class BufferView
  {
    Buffer buf;
    std::shared_ptr<backend::prototypes::BufferViewImpl> impl;
    std::shared_ptr<int64_t> id;
  public:
    BufferView() = default;

    BufferView(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl, std::shared_ptr<int64_t> id)
      : buf(buf)
      , impl(impl)
      , id(id)
    {
    }

    ResourceDescriptor& desc()
    {
      return buf.desc();
    }

    std::shared_ptr<backend::prototypes::BufferViewImpl> native()
    {
      return impl;
    }

    Buffer& buffer()
    {
      return buf;
    }
  };

  class BufferSRV : public BufferView
  {
  public:
    BufferSRV() = default;
    BufferSRV(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl, std::shared_ptr<int64_t> id)
      : BufferView(buf, impl, id)
    {
    }
  };

  class BufferUAV : public BufferView
  {
  public:
    BufferUAV() = default;
    BufferUAV(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl, std::shared_ptr<int64_t> id)
      : BufferView(buf, impl, id)
    {
    }
  };

  class BufferIBV : public BufferView
  {
  public:
    BufferIBV() = default;
    BufferIBV(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl, std::shared_ptr<int64_t> id)
      : BufferView(buf, impl, id)
    {
    }
  };
};