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
    std::shared_ptr<int64_t> m_id;
    std::shared_ptr<ResourceDescriptor> m_desc;

  public:
    Texture()
      : m_desc(std::make_shared<ResourceDescriptor>())
    {

    }

    Texture(std::shared_ptr<backend::prototypes::TextureImpl> impl, std::shared_ptr<int64_t> id, ResourceDescriptor desc)
      : impl(impl)
      , m_id(id)
      , m_desc(std::make_shared<ResourceDescriptor>(std::move(desc)))
    {
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
  };

  class TextureView
  {
    Texture tex;
    std::shared_ptr<backend::prototypes::TextureViewImpl> impl;
    std::shared_ptr<int64_t> id;
  public:
    TextureView() = default;

    TextureView(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id)
      : tex(tex)
      , impl(impl)
      , id(id)
    {

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
  };

  class TextureSRV : public TextureView
  {
  public:

    TextureSRV() = default;
    TextureSRV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id)
      : TextureView(tex, impl, id)
    {
    }
  };

  class TextureUAV : public TextureView
  {
  public:
    TextureUAV() = default;
    TextureUAV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id)
      : TextureView(tex, impl, id)
    {
    }
  };

  class TextureRTV : public TextureView
  {
  public:
    TextureRTV() = default;
    TextureRTV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id)
      : TextureView(tex, impl, id)
    {
    }
  };

  class TextureDSV : public TextureView
  {
  public:
    TextureDSV() = default;
    TextureDSV(Texture tex, std::shared_ptr<backend::prototypes::TextureViewImpl> impl, std::shared_ptr<int64_t> id)
      : TextureView(tex, impl, id)
    {
    }
  };
};