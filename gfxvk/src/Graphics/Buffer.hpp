#pragma once
#include "Descriptors/ResUsage.hpp"
#include "ResourceDescriptor.hpp"
#include "vk_specifics/VulkanBuffer.hpp"
#include <vector>
#include <memory>


// public interace?
class Buffer
{
  friend class GpuDevice;
  friend class BufferShaderView;
  std::shared_ptr<BufferImpl> buffer;

  Buffer()
  {}

  Buffer(BufferImpl impl)
    : buffer(std::make_shared<BufferImpl>(std::forward<decltype(impl)>(impl)))
  {}
public:

  template<typename T>
  MappedBufferImpl<T> Map(int64_t offset, int64_t size)
  {
    return buffer->Map<T>(offset, size);
  }

  ResourceDescriptor desc()
  {
    return buffer->desc();
  }

  BufferImpl& getBuffer()
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
  friend class BufferSRV;
  friend class BufferUAV;
  friend class BufferIBV;
  friend class BufferCBV;

  Buffer m_buffer; // TODO: m_state needs to be synchronized
  BufferShaderViewImpl m_view;
  BufferShaderView()
  {}
public:
  Buffer& buffer()
  {
    return m_buffer;
  }

  bool isValid()
  {
    return m_buffer.isValid() && m_view.isValid();
  }

  size_t getIndexInHeap()
  {
    return m_view.getIndexInHeap(); // This is really confusing getter, for completely wrong reasons.
  }

  unsigned getCustomIndexInHeap() // this returns implementation specific index. There might be better ways to do this.
  {
    return m_view.getCustomIndexInHeap();
  }
};

// to separate different views typewise

class BufferSRV : public BufferShaderView
{

};

class BufferUAV : public BufferShaderView
{

};

class BufferIBV : public BufferShaderView
{

};

class BufferCBV : public BufferShaderView
{

};

