#pragma once
#include "faze/src/new_gfx/desc/Formats.hpp"
#include <string>
#include <array>

namespace faze
{
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
    struct descriptor
    {
      std::string     name = "Unnamed Resource";
      FormatType      format = FormatType::Unknown;
      ResourceUsage   usage = ResourceUsage::GpuReadOnly;
      FormatDimension dimension = FormatDimension::Unknown;
      unsigned        stride = 0;
      unsigned        miplevels = 1;
      unsigned        width = 1;
      unsigned        height = 1;
      unsigned        depth = 1;
      unsigned        arraySize = 1;
      unsigned        msCount = 1;
      bool            index = false;
      bool            indirect = false;
      bool            allowCrossAdapter = false;
      bool            allowSimultaneousAccess = false;
    } desc;

    ResourceDescriptor()
    {
    }

    ResourceDescriptor& setName(std::string name)
    {
      desc.name = name;
      return *this;
    }

    template <typename T>
    ResourceDescriptor& setFormat()
    {
      desc.stride = sizeof(T);
      desc.format = FormatType::Unknown;
      return *this;
    }

    ResourceDescriptor& setFormat(FormatType type)
    {
      desc.stride = 0; // ??
      desc.format = type;
      return *this;
    }

    ResourceDescriptor& setUsage(ResourceUsage usage)
    {
      desc.usage = usage;
      return *this;
    }

    ResourceDescriptor& setDimension(FormatDimension dimension)
    {
      desc.dimension = dimension;
      return *this;
    }

    ResourceDescriptor& setWidth(unsigned size)
    {
      desc.width = size;
      return *this;
    }

    ResourceDescriptor& setHeight(unsigned size)
    {
      desc.height = size;
      return *this;
    }

    ResourceDescriptor& setDepth(unsigned size)
    {
      desc.depth = size;
      return *this;
    }

    ResourceDescriptor& setMiplevels(unsigned levels)
    {
      desc.miplevels = levels;
      return *this;
    }

    ResourceDescriptor& setArraySize(unsigned size = 1)
    {
      desc.arraySize = size;
      return *this;
    }

    ResourceDescriptor& setMultisample(unsigned count = 1)
    {
      desc.msCount = count;
      return *this;
    }

    ResourceDescriptor& setIndexBuffer()
    {
      desc.index = true;
      return *this;
    }

    ResourceDescriptor& setIndirect()
    {
      desc.indirect = true;
      return *this;
    }
    ResourceDescriptor& allowCrossAdapter()
    {
      desc.allowCrossAdapter = true;
      return *this;
    }
    ResourceDescriptor& allowSimultaneousAccess()
    {
      desc.allowSimultaneousAccess = true;
      return *this;
    }
  };
}