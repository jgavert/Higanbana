#pragma once
#include "Descriptors\Formats.hpp"
#include "Descriptors\ResUsage.hpp"
#include <d3d12.h>
#include <string>
#include <array>

// Describes resource
class ResourceDescriptor 
{
public:

  // keep the order
  FormatType m_format;
  ResourceUsage m_usage;
  unsigned m_stride;
  unsigned m_miplevels;
  unsigned m_width;
  unsigned m_height;
  unsigned m_arraySize;
  unsigned m_msCount;
  unsigned m_msQuality;

  ResourceDescriptor()
    : m_format(FormatType::Unknown)
    , m_usage(ResourceUsage::GpuOnly)
    , m_stride(0)
    , m_miplevels(1)
    , m_width(1)
    , m_height(1)
    , m_arraySize(1)
    , m_msCount(1)
    , m_msQuality(0)
	{

	}

  ResourceDescriptor& Miplevels(unsigned levels)
  {
    m_miplevels = levels;
    return *this;
  }
  
  template <typename T>
  ResourceDescriptor& Format()
  {
    m_stride = sizeof(T);
    m_format = FormatType::Unknown;
    return *this;
  }

  ResourceDescriptor& Format(FormatType type)
  {
    m_stride = 0; // ??
    m_format = type;
    return *this;
  }

  ResourceDescriptor& Width(unsigned size)
  {
    m_width = size;
    return *this;
  }

  ResourceDescriptor& Height(unsigned size)
  {
    m_height = size;
    return *this;
  }

  ResourceDescriptor& ArraySize(unsigned size = 1)
  {
    m_arraySize = size;
    return *this;
  }

  ResourceDescriptor& Usage(ResourceUsage usage)
  {
    m_usage = usage;
    return *this;
  }

  ResourceDescriptor& Multisample(unsigned count = 1, unsigned quality = 0)
  {
    m_msCount = count;
    m_msQuality = quality;
    return *this;
  }
};
