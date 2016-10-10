#pragma once
#include "Descriptors/Formats.hpp"
#include "Descriptors/ResUsage.hpp"
#include <string>
#include <array>

enum class ResourceShaderType
{
	Unknown,
	ShaderView,
	UnorderedAccess,
	RenderTarget,
	DepthStencil
};

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
  unsigned m_stride = 0;
  unsigned m_miplevels = 1;
  unsigned m_width = 1;
  unsigned m_height = 1;
  unsigned m_depth = 1;
  unsigned m_arraySize = 1;
  unsigned m_msCount = 1;
  unsigned m_msQuality = 0;
  bool m_rendertarget = false;
  bool m_depthstencil = false;
  bool m_unorderedaccess = false;
  bool m_denysrv = false;
  bool m_allowCrossAdapter = false;
  bool m_allowSimultaneousAccess = false;

  ResourceDescriptor()
    : m_name("Unnamed Resource")
    , m_format(FormatType::Unknown)
    , m_usage(ResourceUsage::GpuOnly)
    , m_dimension(FormatDimension::Unknown)
    , m_layout(TextureLayout::RowMajor)
  {
  }

  ResourceDescriptor& setName(std::string name)
  {
    m_name = name;
    return *this;
  }

  template <typename T>
  ResourceDescriptor& setFormat()
  {
    m_stride = sizeof(T);
    m_format = FormatType::Unknown;
    return *this;
  }

  ResourceDescriptor& setFormat(FormatType type)
  {
    m_stride = 0; // ??
    m_format = type;
    return *this;
  }

  ResourceDescriptor& setUsage(ResourceUsage usage)
  {
    m_usage = usage;
    return *this;
  }

  ResourceDescriptor& setDimension(FormatDimension dimension)
  {
    m_dimension = dimension;
    return *this;
  }

  ResourceDescriptor& setLayout(TextureLayout layout)
  {
    m_layout = layout;
    return *this;
  }

  ResourceDescriptor& setWidth(unsigned size)
  {
    m_width = size;
    return *this;
  }

  ResourceDescriptor& setHeight(unsigned size)
  {
    m_height = size;
    return *this;
  }

  ResourceDescriptor& setDepth(unsigned size)
  {
    m_depth = size;
    return *this;
  }

  ResourceDescriptor& setMiplevels(unsigned levels)
  {
    m_miplevels = levels;
    return *this;
  }

  ResourceDescriptor& setArraySize(unsigned size = 1)
  {
    m_arraySize = size;
    return *this;
  }

  ResourceDescriptor& setMultisample(unsigned count = 1, unsigned quality = 0)
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
