#pragma once

#include "graphics/common/backend.hpp"
#include "graphics/common/resources.hpp"
#include "graphics/common/resource_descriptor.hpp"
#include "graphics/common/prototypes.hpp"
#include "graphics/common/handle.hpp"

namespace faze
{
  class Buffer
  {
    std::shared_ptr<ResourceHandle> m_id;
    std::shared_ptr<ResourceDescriptor> m_desc;
  public:
    Buffer()
      : m_id(std::make_shared<ResourceHandle>(InvalidResourceHandle))
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
      return *m_id;
    }
  };

  class BufferView
  {
    Buffer buf;
    std::shared_ptr<ResourceHandle> m_id;
  public:
    BufferView() = default;

    BufferView(Buffer buf, std::shared_ptr<ResourceHandle> id)
      : buf(buf)
      , m_id(id)
    {
    }

    ResourceDescriptor& desc() const
    {
      return buf.desc();
    }

    ResourceHandle handle() const
    {
      return *m_id;
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
    BufferSRV(Buffer buf, std::shared_ptr<ResourceHandle> id)
      : BufferView(buf, id)
    {
    }
  };

  class BufferUAV : public BufferView
  {
  public:
    BufferUAV() = default;
    BufferUAV(Buffer buf, std::shared_ptr<ResourceHandle> id)
      : BufferView(buf, id)
    {
    }
  };

  class BufferIBV : public BufferView
  {
  public:
    BufferIBV() = default;
    BufferIBV(Buffer buf, std::shared_ptr<ResourceHandle> id)
      : BufferView(buf, id)
    {
    }
  };

  class DynamicBufferView
  {
    ResourceHandle m_id;

  public:
    DynamicBufferView() = default;
    DynamicBufferView(ResourceHandle id)
      : m_id(id)
    {
    }
    ResourceHandle handle() const
    {
      return m_id;
    }
  };
};