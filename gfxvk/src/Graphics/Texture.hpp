#pragma once
#include "Descriptors/ResUsage.hpp"
#include "ResourceDescriptor.hpp"
#include "vk_specifics/VulkanTexture.hpp"
#include <vector>
#include <memory>


// public interace?
class Texture
{
  friend class GpuDevice;
  friend class TextureShaderView;
  std::shared_ptr<TextureImpl> texture;
  std::shared_ptr<ResourceDescriptor> m_desc;

  Texture()
  {}
  Texture(TextureImpl impl, ResourceDescriptor desc)
    : texture(std::make_shared<TextureImpl>(std::forward<decltype(impl)>(impl)))
    , m_desc(std::make_shared<ResourceDescriptor>(std::forward<ResourceDescriptor>(desc)))
  {}

public:

  template<typename type>
  MappedTextureImpl<type> Map()
  {
    return texture->Map<type>();
  }

  ResourceDescriptor& desc()
  {
    return *m_desc;
  }

  TextureImpl& getTexture()
  {
    return *texture;
  }

  bool isValid()
  {
    return texture->isValid();
  }
};

class TextureShaderView
{
private:
  friend class GpuDevice;
  friend class GraphicsCmdBuffer;
  friend class TextureSRV;
  friend class TextureUAV;
  friend class TextureRTV;
  friend class TextureDSV;

  Texture m_texture;
  TextureShaderViewImpl m_view;

  TextureShaderView()
  {}
public:

  Texture& texture()
  {
    return m_texture;
  }

  bool isValid()
  {
    return m_texture.isValid();
  }

  TextureShaderViewImpl& getView()
  {
	  return m_view;
  }
};

// to separate different views typewise, also enables information to know from which view we are trying to move into.

class TextureSRV : public TextureShaderView
{

};

class TextureUAV : public TextureShaderView
{

};

class TextureRTV : public TextureShaderView
{

};

class TextureDSV : public TextureShaderView
{

};
