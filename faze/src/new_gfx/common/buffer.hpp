#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "prototypes.hpp"

namespace faze
{
  class Buffer
  {
    std::shared_ptr<backend::prototypes::BufferImpl> impl;
    std::shared_ptr<int64_t> id;
    std::shared_ptr<ResourceDescriptor> m_desc;
    backend::TrackedState m_dependency;
  public:
    Buffer()
      : m_desc(std::make_shared<ResourceDescriptor>())
    {
    }
    Buffer(std::shared_ptr<backend::prototypes::BufferImpl> impl, std::shared_ptr<int64_t> id, ResourceDescriptor desc)
      : impl(impl)
      , id(id)
      , m_desc(std::make_shared<ResourceDescriptor>(std::move(desc)))
      , m_dependency(impl->dependency())
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
    backend::TrackedState dependency()
    {
      return m_dependency;
    }
  };

  class BufferView
  {
    Buffer buf;
    std::shared_ptr<backend::prototypes::BufferViewImpl> impl;
    std::shared_ptr<int64_t> id;
    backend::RawView m_view;
    backend::TrackedState m_dependency;
  public:
    BufferView() = default;

    BufferView(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl, std::shared_ptr<int64_t> id)
      : buf(buf)
      , impl(impl)
      , id(id)
      , m_view(impl->view())
      , m_dependency(buf.dependency())
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

    backend::RawView view()
    {
      return m_view;
    }
    backend::TrackedState dependency()
    {
      return m_dependency;
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

  class DynamicBufferView
  {
    std::shared_ptr<backend::prototypes::DynamicBufferViewImpl> impl;
    backend::RawView m_view;
  public:
    DynamicBufferView() = default;
    DynamicBufferView(std::shared_ptr<backend::prototypes::DynamicBufferViewImpl> impl)
      : impl(impl)
      , m_view(impl->view())
    {
    }
    backend::RawView view() const
    {
      return m_view;
    }
  };
};