#pragma once
#include "Descriptors/ResUsage.hpp"
#include "RawResource.hpp"
#include "ResourceDescriptor.hpp"
#include "core/src/memory/ManagedResource.hpp"

#include <memory>

template<typename type>
struct MappedTexture
{
  std::shared_ptr<RawResource> m_mappedresource;
  type* mapped;

  MappedTexture(type* ptr)
    : m_mappedresource(nullptr)
    , mapped(ptr)
  {}

  MappedTexture(std::shared_ptr<RawResource> res, type* ptr)
    : m_mappedresource(res)
    , mapped(ptr)
  {}

  size_t rangeBegin()
  {
    return 0;
  }

  size_t rangeEnd()
  {
    return 0;
  }

  type& operator[](size_t i)
  {
    return mapped[i];
  }

  type* get()
  {
    return mapped;
  }

  // should always check if mapping is valid.
  bool valid()
  {
    return false;
  }
};

struct TextureInternal
{
  std::shared_ptr<RawResource> m_resource; // SpecialWrapping so that we can take "not owned" objects as our resources.
  ResourceDescriptor m_desc;

  template<typename T>
  MappedTexture<T> Map()
  {
    return MappedTexture<T>(nullptr);
  }

  bool isValid()
  {
	  return true;
  }
};

// public interace?
class Texture
{
  friend class GpuDevice;
  std::shared_ptr<TextureInternal> texture;
public:

  template<typename type>
  MappedTexture<type> Map()
  {
    return texture->Map<type>();
  }

  TextureInternal& getTexture()
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
  Texture m_texture; // keep texture alive here, if copying is issue like it could be. TODO: REFACTOR
  FazPtr<size_t> indexInHeap; // will handle removing references from heaps when destructed. ref counted.
  size_t customIndex;
public:
  TextureInternal& texture()
  {
    return m_texture.getTexture();
  }

  bool isValid()
  {
    return m_texture.getTexture().isValid();
  }

  size_t getIndexInHeap()
  {
    return *indexInHeap.get(); // This is really confusing getter, for completely wrong reasons.
  }

  unsigned getCustomIndexInHeap() // this returns implementation specific index. There might be better ways to do this.
  {
    return static_cast<unsigned>(customIndex);
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
