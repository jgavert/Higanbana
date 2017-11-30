#pragma once
#include "faze/src/new_gfx/desc/Formats.hpp"
#include "core/src/math/vec_templated.hpp"
#include <string>
#include <array>

namespace faze
{
  // todo move this to somewhere else.
  struct SubresourceRange
  {
    static constexpr const int16_t WholeResource = -1;
    int16_t mipOffset = -1;
    int16_t mipLevels = -1;
    int16_t sliceOffset = -1;
    int16_t arraySize = -1;
  private:
    inline bool overlapRange(int16_t xoffset, int16_t xsize, int16_t yoffset, int16_t ysize)
    {
      return xoffset < yoffset + ysize && yoffset < xoffset + xsize;
    }
  public:
    bool overlap(SubresourceRange other)
    {
      if (mipOffset == SubresourceRange::WholeResource || other.mipOffset == SubresourceRange::WholeResource)
        return true;
      return overlapRange(mipOffset, mipLevels, other.mipOffset, other.mipLevels) // if mips don't overlap, no hope.
        && overlapRange(sliceOffset, arraySize, other.sliceOffset, other.arraySize); // otherwise also arrays need to match.
    }
  };

  enum class PresentMode
  {
    Unknown,
    Immediate,
    Mailbox,
    Fifo,
    FifoRelaxed
  };

  enum class Colorspace
  {
    BT709,
    BT2020
  };

  enum DisplayCurve
  {
    sRGB = 0,	// The display expects an sRGB signal.
    ST2084,		// The display expects an HDR10 signal.
    None		// The display expects a linear signal.
  };

  const char* presentModeToStr(PresentMode mode);
  const char* colorspaceToStr(Colorspace mode);
  const char* displayCurveToStr(DisplayCurve mode);

  class SwapchainDescriptor
  {
  public:
    struct descriptor
    {
      PresentMode mode = PresentMode::Fifo;
      FormatType format = FormatType::Unorm8RGBA;
      int bufferCount = 2;
      Colorspace colorSpace = Colorspace::BT709;
    } desc;
    SwapchainDescriptor()
    {}
    SwapchainDescriptor& presentMode(PresentMode mode)
    {
      desc.mode = mode;
      return *this;
    }
    SwapchainDescriptor& formatType(FormatType format)
    {
      desc.format = format;
      return *this;
    }
    SwapchainDescriptor& bufferCount(unsigned count)
    {
      desc.bufferCount = count;
      return *this;
    }
    SwapchainDescriptor& colorspace(Colorspace space)
    {
      desc.colorSpace = space;
      return *this;
    }
  };

  enum class ResourceShaderType
  {
    Unknown,
    ReadOnly,
    ReadWrite,
    RenderTarget,
    DepthStencil
  };

  class ShaderViewDescriptor
  {
  public:
    unsigned m_mostDetailedMip = 0;
    unsigned m_arraySlice = 0;
    int m_mipLevels = -1;
    int m_arraySize = -1;
    unsigned m_planeSlice = 0;
    unsigned m_first2DArrayFace = 0;
    unsigned m_firstElement = 0;
    int m_elementCount = -1;
    float m_resourceMinLODClamp = 0.f;
    FormatType m_format = FormatType::Unknown; // basically, look at base descriptor.
    ResourceShaderType m_viewType = ResourceShaderType::ReadOnly;

    ShaderViewDescriptor()
    {}
    ShaderViewDescriptor& setMostDetailedMip(unsigned index)
    {
      m_mostDetailedMip = index;
      return *this;
    }
    ShaderViewDescriptor& setArraySlice(unsigned index)
    {
      m_arraySlice = index;
      return *this;
    }
    ShaderViewDescriptor& setMipLevels(unsigned index)
    {
      m_mipLevels = index;
      return *this;
    }
    ShaderViewDescriptor& setArraySize(unsigned index)
    {
      m_arraySize = index;
      return *this;
    }
    ShaderViewDescriptor& setPlaneSlice(unsigned index)
    {
      m_planeSlice = index;
      return *this;
    }
    ShaderViewDescriptor& setFirst2DArrayFace(unsigned index)
    {
      m_first2DArrayFace = index;
      return *this;
    }
    ShaderViewDescriptor& setFirstElement(unsigned index)
    {
      m_firstElement = index;
      return *this;
    }
    ShaderViewDescriptor& setElementCount(int value)
    {
      m_elementCount = value;
      return *this;
    }
    ShaderViewDescriptor& setResourceMinLODClamp(float clamp)
    {
      m_resourceMinLODClamp = clamp;
      return *this;
    }
    ShaderViewDescriptor& setFormat(FormatType type)
    {
      m_format = type;
      return *this;
    }
    ShaderViewDescriptor& setType(ResourceShaderType type)
    {
      m_viewType = type;
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

    // convert this to be solely for structured buffer... actually do it now since it doesn't make sense otherwise.
    template <typename T>
    ResourceDescriptor& setStructured()
    {
      desc.stride = sizeof(T);
      desc.format = FormatType::Unknown;
      return *this;
    }

    ResourceDescriptor& setFormat(FormatType type)
    {
      desc.stride = formatSizeInfo(type).pixelSize;
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

    ResourceDescriptor& setSize(int3 size)
    {
      desc.width = size.x();
      desc.height = size.y();
      desc.depth = size.z();
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