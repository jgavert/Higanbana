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
  std::shared_ptr<ResourceDescriptor> m_desc;

  Buffer()
  {}

  Buffer(BufferImpl impl, ResourceDescriptor desc)
    : buffer(std::make_shared<BufferImpl>(std::forward<decltype(impl)>(impl)))
    , m_desc(std::make_shared<ResourceDescriptor>(std::forward<ResourceDescriptor>(desc)))
  {}
public:

  template<typename T>
  MappedBufferImpl<T> Map(int64_t offset, int64_t size)
  {
    return buffer->Map<T>(offset*sizeof(T), size*sizeof(T));
  }

  ResourceDescriptor& desc()
  {
    return *m_desc;
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

  Buffer m_buffer;
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
    return m_buffer.isValid();
  }

  BufferShaderViewImpl& getView()
  {
	  return m_view;
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

