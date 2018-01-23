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
    Buffer(std::shared_ptr<backend::prototypes::BufferImpl> impl, std::shared_ptr<int64_t> id, std::shared_ptr<ResourceDescriptor> desc)
      : impl(impl)
      , id(id)
      , m_desc(desc)
      , m_dependency(impl->dependency())
    {
      m_dependency.storeSubresourceRange(
        static_cast<unsigned>(0),
        static_cast<unsigned>(1),
        static_cast<unsigned>(0),
        static_cast<unsigned>(1));
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
    backend::RawView m_view;
    backend::TrackedState m_dependency;
  public:
    BufferView() = default;

    BufferView(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl)
      : buf(buf)
      , impl(impl)
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

    backend::RawView view() const
    {
      return m_view;
    }
    backend::TrackedState dependency() const
    {
      return m_dependency;
    }
  };

  class BufferSRV : public BufferView
  {
  public:
    BufferSRV() = default;
    BufferSRV(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl)
      : BufferView(buf, impl)
    {
    }
  };

  class BufferUAV : public BufferView
  {
  public:
    BufferUAV() = default;
    BufferUAV(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl)
      : BufferView(buf, impl)
    {
    }
  };

  class BufferIBV : public BufferView
  {
  public:
    BufferIBV() = default;
    BufferIBV(Buffer buf, std::shared_ptr<backend::prototypes::BufferViewImpl> impl)
      : BufferView(buf, impl)
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

    int rowPitch() const
    {
      return impl->rowPitch();
    }

    int64_t offset() const
    {
      return impl->offset();
    }

    std::shared_ptr<backend::prototypes::DynamicBufferViewImpl> native()
    {
      return impl;
    }
  };

  class SharedBuffer
  {
    std::shared_ptr<backend::prototypes::BufferImpl> primaryImpl;
    std::shared_ptr<backend::prototypes::BufferImpl> secondaryImpl;
    std::shared_ptr<int64_t> id;
    std::shared_ptr<ResourceDescriptor> m_desc;
  public:
    SharedBuffer()
      : m_desc(std::make_shared<ResourceDescriptor>())
    {
    }
    SharedBuffer(std::shared_ptr<backend::prototypes::BufferImpl> primaryImpl, std::shared_ptr<backend::prototypes::BufferImpl> secondaryImpl, std::shared_ptr<int64_t> id, std::shared_ptr<ResourceDescriptor> desc)
      : primaryImpl(primaryImpl)
      , secondaryImpl(secondaryImpl)
      , id(id)
      , m_desc(desc)
    {
    }

    Buffer getPrimaryBuffer()
    {
      return Buffer(primaryImpl, id, m_desc);
    }

    Buffer getSecondaryBuffer()
    {
      return Buffer(secondaryImpl, id, m_desc);
    }

    ResourceDescriptor& desc()
    {
      return *m_desc;
    }
  };
};