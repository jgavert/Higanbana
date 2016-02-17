#pragma once
#include "Descriptors/ResUsage.hpp"
#include "ResourceDescriptor.hpp"
#include <vector>

template<typename type>
struct MappedBuffer
{
  void* m_mappedresource;
  type mapped;
  MappedBuffer(void* res)
    : m_mappedresource(res)
  {}
  ~MappedBuffer()
  {
  }

  size_t rangeBegin()
  {
    return 0;
  }

  size_t rangeEnd()
  {
    return 0;
  }

  type& operator[](size_t)
  {
    return mapped;
  }

  type* get()
  {
    return &mapped;
  }

  bool isValid()
  {
    return false;
  }
};

struct BufferView
{
private:
  size_t index;
public:
  size_t getCpuHandle()
  {
    return index;
  }
  size_t getGpuHandle()
  {
    return index;
  }
};

struct Buffer
{
  void* m_resource;

  template<typename T>
  MappedBuffer<T> Map()
  {
    return MappedBuffer<T>(m_resource);
  }

  bool isValid()
  {
    return true;
  }
};

class _Buffer
{
private:

  Buffer m_buffer;
public:
  Buffer& buffer() { return m_buffer; }

  bool isValid()
  {
    return m_buffer.isValid();
  }
};

class BufferSRV : public _Buffer
{
public:
 
};

class BufferUAV : public _Buffer
{
public:

};

class BufferIBV : public _Buffer
{
public:

};

class BufferCBV : public _Buffer
{
public:

};

//////////////////////////////////////////////////////////////////
// New stuff

struct Buffer_new
{
  void* m_resource; 
  ResourceDescriptor m_desc;


  template<typename T>
  MappedBuffer<T> Map()
  {
    return MappedBuffer<T>(m_resource, ptr);
  }

  bool isValid()
  {
    return true;
  }
};
