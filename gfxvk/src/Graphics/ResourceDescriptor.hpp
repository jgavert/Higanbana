#pragma once
#include "Descriptors/Formats.hpp"
#include "Descriptors/ResUsage.hpp"
#include <string>
#include <array>

class ShaderViewDescriptor
{
public:
  unsigned m_mostDetailedMip = 0;
  unsigned m_arraySlice = 0;
  unsigned m_planeSlice = 0;
  unsigned m_first2DArrayFace = 0;
  unsigned m_firstElement = 0;
  int m_elementCount = -1;
  float m_resourceMinLODClamp = 0.f;
  ShaderViewDescriptor()
  {}
  ShaderViewDescriptor& MostDetailedMip(unsigned index)
  {
    m_mostDetailedMip = index;
    return *this;
  }
  ShaderViewDescriptor& ArraySlice(unsigned index)
  {
    m_arraySlice = index;
    return *this;
  }
  ShaderViewDescriptor& PlaneSlice(unsigned index)
  {
    m_planeSlice = index;
    return *this;
  }
  ShaderViewDescriptor& First2DArrayFace(unsigned index)
  {
    m_first2DArrayFace = index;
    return *this;
  }
  ShaderViewDescriptor& FirstElement(unsigned index)
  {
    m_firstElement = index;
    return *this;
  }
  ShaderViewDescriptor& ElementCount(int value)
  {
	m_elementCount = value;
	return *this;
  }
  ShaderViewDescriptor& ResourceMinLODClamp(float clamp)
  {
    m_resourceMinLODClamp = clamp;
    return *this;
  }
};

// Describes resource
class ResourceDescriptor
{
public:
  // keep the order
  std::string m_name;
  FormatType m_format;
  ResourceUsage m_usage;
  FormatDimension m_dimension;
  TextureLayout m_layout;
  unsigned m_stride;
  unsigned m_miplevels;
  unsigned m_width;
  unsigned m_height;
  unsigned m_arraySize;
  unsigned m_msCount;
  unsigned m_msQuality;
  bool m_rendertarget;
  bool m_depthstencil;
  bool m_unorderedaccess;
  bool m_denysrv;
  bool m_allowCrossAdapter;
  bool m_allowSimultaneousAccess;

  ResourceDescriptor()
    : m_name("Unnamed Resource")
    , m_format(FormatType::Unknown)
    , m_usage(ResourceUsage::GpuOnly)
    , m_dimension(FormatDimension::Unknown)
    , m_layout(TextureLayout::RowMajor)
    , m_stride(0)
    , m_miplevels(1)
    , m_width(1)
    , m_height(1)
    , m_arraySize(1)
    , m_msCount(1)
    , m_msQuality(0)
    , m_rendertarget(false)
    , m_depthstencil(false)
    , m_unorderedaccess(false)
    , m_denysrv(false)
    , m_allowCrossAdapter(false)
    , m_allowSimultaneousAccess(false)
  {
  }

  ResourceDescriptor& Name(std::string name)
  {
    m_name = name;
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

  ResourceDescriptor& Usage(ResourceUsage usage)
  {
    m_usage = usage;
    return *this;
  }

  ResourceDescriptor& Dimension(FormatDimension dimension)
  {
    m_dimension = dimension;
    return *this;
  }

  ResourceDescriptor& Layout(TextureLayout layout)
  {
    m_layout = layout;
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

  ResourceDescriptor& Miplevels(unsigned levels)
  {
    m_miplevels = levels;
    return *this;
  }

  ResourceDescriptor& ArraySize(unsigned size = 1)
  {
    m_arraySize = size;
    return *this;
  }

  ResourceDescriptor& Multisample(unsigned count = 1, unsigned quality = 0)
  {
    m_msCount = count;
    m_msQuality = quality;
    return *this;
  }

  ResourceDescriptor& enableRendertarget()
  {
    m_rendertarget = true;
    return *this;
  }
  ResourceDescriptor& enableDepthStencil()
  {
    m_depthstencil = true;
    return *this;
  }
  ResourceDescriptor& enableUnorderedAccess()
  {
    m_unorderedaccess = true;
    return *this;
  }
  ResourceDescriptor& denyShaderAccess()
  {
    m_denysrv = true;
    return *this;
  }
  ResourceDescriptor& allowCrossAdapter()
  {
    m_allowCrossAdapter = true;
    return *this;
  }
  ResourceDescriptor& allowSimultaneousAccess()
  {
    m_allowSimultaneousAccess = true;
    return *this;
  }
};
