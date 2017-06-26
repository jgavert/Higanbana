#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "prototypes.hpp"

namespace faze
{
  class Texture
  {
    std::shared_ptr<backend::prototypes::TextureImpl> impl;
    std::shared_ptr<int64_t> m_id;
    std::shared_ptr<ResourceDescriptor> m_desc;
    backend::TrackedState m_dependency;
  public:
    Texture()
      : m_desc(std::make_shared<ResourceDescriptor>())
    {
    }

    Texture(std::shared_ptr<backend::prototypes::TextureImpl> impl, std::shared_ptr<int64_t> id, ResourceDescriptor desc)
      : impl(impl)
      , m_id(id)
      , m_desc(std::make_shared<ResourceDescriptor>(std::move(desc)))
      , m_dependency(impl->dependency())
    {
      m_dependency.storeSubresourceRange(0, m_desc->desc.miplevels, 0, m_desc->desc.arraySize);
    }

    ResourceDescriptor& desc()
    {
      return *m_desc;
    }

    std::shared_ptr<backend::prototypes::TextureImpl> native()
    {
      return impl;
    }

    int64_t id()
    {
      return *m_id;
    }

    backend::TrackedState dependency()
    {
      return m_dependency;
    }
  };

  class TextureView
  {
    Texture tex;
    std::shared_ptr<backend::prototypes::TextureViewImpl> impl;
    std::shared_ptr<int64_t> m_id;
    backend::RawView m_view;
    backend::TrackedState m_dependency;

    LoadOp m_loadOp = LoadOp::Load;
    StoreOp m_storeOp = StoreOp::Store;
  public:
    TextureView() = default;

    TextureView(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id, const SubresourceRange& range)
      : tex(tex)
      , impl(impl)
      , m_id(id)
      , m_view(impl->view())
      , m_dependency(tex.dependency())
    {
      F_LOG("Storing data %u %u %u %u\n", static_cast<unsigned>(range.mipOffset),
        static_cast<unsigned>(range.mipLevels),
        static_cast<unsigned>(range.sliceOffset),
        static_cast<unsigned>(range.arraySize));
      m_dependency.storeSubresourceRange(
        static_cast<unsigned>(range.mipOffset),
        static_cast<unsigned>(range.mipLevels),
        static_cast<unsigned>(range.sliceOffset),
        static_cast<unsigned>(range.arraySize));
      F_LOG("verify data %u %u %u %u\n",
        m_dependency.mip(),
        m_dependency.mipLevels(),
        m_dependency.slice(),
        m_dependency.arraySize());
    }

    ResourceDescriptor& desc()
    {
      return tex.desc();
    }

    std::shared_ptr<backend::prototypes::TextureViewImpl> native()
    {
      return impl;
    }

    Texture& texture()
    {
      return tex;
    }

    int64_t id()
    {
      return *m_id;
    }

    void setOp(LoadOp op)
    {
      m_loadOp = op;
    }

    void setOp(StoreOp op)
    {
      m_storeOp = op;
    }

    LoadOp loadOp()
    {
      return m_loadOp;
    }

    StoreOp storeOp()
    {
      return m_storeOp;
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

  class TextureSRV : public TextureView
  {
  public:

    TextureSRV() = default;
    TextureSRV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id, const SubresourceRange& range)
      : TextureView(tex, impl, id, range)
    {
    }

    TextureSRV& op(LoadOp op)
    {
      setOp(op);
      return *this;
    }

    TextureSRV& op(StoreOp op)
    {
      setOp(op);
      return *this;
    }
  };

  class TextureUAV : public TextureView
  {
  public:
    TextureUAV() = default;
    TextureUAV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id, const SubresourceRange& range)
      : TextureView(tex, impl, id, range)
    {
    }

    TextureUAV& op(LoadOp op)
    {
      setOp(op);
      return *this;
    }

    TextureUAV& op(StoreOp op)
    {
      setOp(op);
      return *this;
    }
  };

  class TextureRTV : public TextureView
  {
  public:
    TextureRTV() = default;
    TextureRTV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id, const SubresourceRange& range)
      : TextureView(tex, impl, id, range)
    {
    }

    TextureRTV& op(LoadOp op)
    {
      setOp(op);
      return *this;
    }

    TextureRTV& op(StoreOp op)
    {
      setOp(op);
      return *this;
    }
  };

  class TextureDSV : public TextureView
  {
  public:
    TextureDSV() = default;
    TextureDSV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id, const SubresourceRange& range)
      : TextureView(tex, impl, id, range)
    {
    }

    TextureDSV& op(LoadOp op)
    {
      setOp(op);
      return *this;
    }

    TextureDSV& op(StoreOp op)
    {
      setOp(op);
      return *this;
    }
  };
};