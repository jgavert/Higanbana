#pragma once
#include "Descriptors/ResUsage.hpp"
#include "ResourceDescriptor.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include <vector>
#include <memory>

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


struct BufferInternal
{
  void* m_resource;
  ResourceDescriptor m_desc;


  template<typename T>
  MappedBuffer<T> Map()
  {
    return MappedBuffer<T>(nullptr);
  }

  bool isValid()
  {
    return true;
  }
};

// public interace?
class Buffer
{
  friend class GpuDevice;
  std::shared_ptr<BufferInternal> buffer;
public:

  template<typename T>
  MappedBuffer<T> Map()
  {
    return buffer->Map<T>();
  }

  BufferInternal& getBuffer()
  {
    return *buffer;
  }

  bool isValid()
  {
    return buffer->isValid();
  }
};

class BufferShaderView
{
private:
  friend class Binding_;
  friend class GpuDevice;
  Buffer m_buffer; // TODO: m_state needs to be synchronized
  FazPtr<size_t> indexInHeap; // will handle removing references from heaps when destructed. ref counted.
  size_t customIndex;
public:
  Buffer& buffer()
  {
    return m_buffer;
  }

  bool isValid()
  {
    return m_buffer.isValid();
  }

  BufferInternal& getBuffer()
  {
    return m_buffer.getBuffer();
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

// to separate different views typewise

class BufferSRV : public BufferShaderView
{
public:

};

class BufferUAV : public BufferShaderView
{
public:

};

class BufferIBV : public BufferShaderView
{
public:

};

class BufferCBV : public BufferShaderView
{
public:

};

