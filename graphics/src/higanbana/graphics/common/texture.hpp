#pragma once

#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include <algorithm>
namespace higanbana
{
  class Texture
  {
    std::shared_ptr<ResourceHandle> m_id;
    std::shared_ptr<ResourceDescriptor> m_desc;
  public:
    Texture()
      : m_id(std::make_shared<ResourceHandle>())
      , m_desc(std::make_shared<ResourceDescriptor>())
    {
    }

    Texture(const Texture&) = default;
    Texture(Texture&&) = default;
    Texture& operator=(const Texture&) = default;
    Texture& operator=(Texture&&) = default;

    ~Texture()
    {
    }

    Texture(std::shared_ptr<ResourceHandle> id, std::shared_ptr<ResourceDescriptor> desc)
      : m_id(id)
      , m_desc(desc)
    {
    }

    ResourceDescriptor& desc()
    {
      return *m_desc;
    }

    uint3 size3D() {
      return desc().desc.size3D();
    }

    FormatType format() {
      return desc().desc.format;
    }

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };

  class TextureView
  {
    Texture tex;
    std::shared_ptr<ViewResourceHandle> m_id;

    LoadOp m_loadOp = LoadOp::Load;
    StoreOp m_storeOp = StoreOp::Store;
    float4 clearColor;
  public:
    TextureView() = default;

    TextureView(Texture tex, std::shared_ptr<ViewResourceHandle> id)
      : tex(tex)
      , m_id(id)
    {
    }

    ResourceDescriptor& desc()
    {
      return tex.desc();
    }

    Texture& texture()
    {
      return tex;
    }

    uint mip() {
      return m_id->startMip();
    }

    uint slice() {
      return m_id->startArr();
    }

    uint3 size3D() {
      auto smip = mip();
      auto size = texture().size3D();
      while (smip > 0) {
        size = uint3(std::max(1u, size.x / 2), std::max(1u, size.y / 2), std::max(1u, size.z / 2));
        smip--;
      }
      return size;
    }

    ViewResourceHandle handle() const
    {
      if (m_id)
      {
        auto h = *m_id;
        h.setLoadOp(m_loadOp);
        h.setStoreOp(m_storeOp);
        return h;
      }
      return ViewResourceHandle();
    }

    void clearOp(float4 clearVal)
    {
      m_loadOp = LoadOp::Clear;
      clearColor = clearVal;
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

    float4 clearVal()
    {
      return clearColor;
    }

    StoreOp storeOp()
    {
      return m_storeOp;
    }
  };

  class TextureSRV : public TextureView
  {
  public:

    TextureSRV() = default;
    TextureSRV(Texture tex, std::shared_ptr<ViewResourceHandle> id)
      : TextureView(tex, id)
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
    TextureUAV(Texture tex, std::shared_ptr<ViewResourceHandle> id)
      : TextureView(tex, id)
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
    TextureRTV(Texture tex, std::shared_ptr<ViewResourceHandle> id)
      : TextureView(tex, id)
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
    TextureDSV(Texture tex, std::shared_ptr<ViewResourceHandle> id)
      : TextureView(tex, id)
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