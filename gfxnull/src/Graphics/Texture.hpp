#pragma once
#include "Descriptors/ResUsage.hpp"
#include "RawResource.hpp"
#include "ResourceDescriptor.hpp"

#include <memory>

template<typename type>
struct MappedTexture // TODO: add support for "invalid mapping".
{
  void* m_mappedresource;
  type* mapped;
  MappedTexture(void* res, type* ptr)
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

  bool isValid()
  {
    return false;
  }
};

struct TextureView
{
private:
  friend class GpuDevice;
  size_t index;
public:
	TextureView()
	{
		index = 0;
	}
  size_t& getCpuHandle()
  {
    return index;
  }
  size_t& getGpuHandle()
  {
    return index;
  }
  size_t getIndex()
  {
    return index;
  }
};

struct Texture
{
  void* m_resource;
  unsigned shader_heap_index;
  template<typename T>
  MappedTexture<T> Map()
  {
    return MappedTexture<T>(m_resource, m_resource);
  }
};

class _Texture
{
private:

  Texture m_texture;
public:
  Texture& texture() { return m_texture; }
  bool isValid()
  {
    return true;
  }
};

class TextureSRV : public _Texture
{

};

class TextureUAV : public _Texture
{

};

class TextureRTV : public _Texture
{
  friend class GpuDevice;
  void* m_scRTV;
public:
  void* textureRTV() { return m_scRTV; }
};

class TextureDSV : public _Texture
{

};

//////////////////////////////////////////////////////////////////
// New stuff

template<typename type>
struct MappedTexture_new
{
  std::shared_ptr<RawResource> m_mappedresource;
  type* mapped;

  MappedTexture_new(type* ptr)
    : m_mappedresource(nullptr)
    , mapped(ptr)
  {}

  MappedTexture_new(std::shared_ptr<RawResource> res, type* ptr)
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

struct Texture_new
{
  std::shared_ptr<RawResource> m_resource; // SpecialWrapping so that we can take "not owned" objects as our resources.
  ResourceDescriptor m_desc;

  template<typename T>
  MappedTexture<T> Map()
  {
    return MappedTexture_new<T>(nullptr);
  }

  bool isValid()
  {
	  return true;
  }
};
