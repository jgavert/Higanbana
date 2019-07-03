#pragma once

#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/common/handle.hpp"

namespace higanbana
{
  class Buffer
  {
    std::shared_ptr<ResourceHandle> m_id;
    std::shared_ptr<ResourceDescriptor> m_desc;
  public:
    Buffer()
      : m_id(std::make_shared<ResourceHandle>())
      , m_desc(std::make_shared<ResourceDescriptor>())
    {
    }
    Buffer(std::shared_ptr<ResourceHandle> id, std::shared_ptr<ResourceDescriptor> desc)
      : m_id(id)
      , m_desc(desc)
    {
    }

    ResourceDescriptor& desc() const
    {
      return *m_desc;
    }

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };

  class BufferView
  {
    Buffer buf;
    std::shared_ptr<ViewResourceHandle> m_id;
  public:
    BufferView() = default;

    BufferView(Buffer buf, std::shared_ptr<ViewResourceHandle> id)
      : buf(buf)
      , m_id(id)
    {
    }

    ResourceDescriptor& desc() const
    {
      return buf.desc();
    }

    ViewResourceHandle handle() const
    {
       if (m_id)
        return *m_id;
      return ViewResourceHandle();
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
    BufferSRV(Buffer buf, std::shared_ptr<ViewResourceHandle> id)
      : BufferView(buf, id)
    {
    }
  };

  class BufferUAV : public BufferView
  {
  public:
    BufferUAV() = default;
    BufferUAV(Buffer buf, std::shared_ptr<ViewResourceHandle> id)
      : BufferView(buf, id)
    {
    }
  };

  class BufferIBV : public BufferView
  {
  public:
    BufferIBV() = default;
    BufferIBV(Buffer buf, std::shared_ptr<ViewResourceHandle> id)
      : BufferView(buf, id)
    {
    }
  };

  class DynamicBufferView
  {
    ViewResourceHandle m_id;

  public:
    DynamicBufferView()
    {

    }
    DynamicBufferView(ViewResourceHandle id)
      : m_id(id)
    {
    }
    ViewResourceHandle handle() const
    {
      return m_id;
    }
  };
};