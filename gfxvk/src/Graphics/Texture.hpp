#pragma once
#include "Descriptors/ResUsage.hpp"
#include "RawResource.hpp"
#include "ResourceDescriptor.hpp"
#include "core/src/memory/ManagedResource.hpp"

#include <memory>
//////////////////////////////////////////////////////////////////
// New stuff

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
