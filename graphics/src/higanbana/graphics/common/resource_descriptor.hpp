#pragma once
#include "higanbana/graphics/desc/formats.hpp"
#include "higanbana/graphics/common/resources/graphics_api.hpp"
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/system/memview.hpp>
#include <string>
#include <array>
#include <algorithm>

namespace higanbana
{
  // todo move this to somewhere else.
  struct Subresource
  {
    int16_t mipLevel = 0;
    int16_t arraySlice = 0;

    Subresource() {}
    Subresource& mip(int mip)
    {
      mipLevel = static_cast<int16_t>(mip);
      return *this;
    }
    Subresource& slice(int slice)
    {
      arraySlice = static_cast<int16_t>(slice);
      return *this;
    }
  };

  struct SubresourceRange
  {
    static constexpr const int16_t WholeResource = -1;
    int16_t mipOffset = -1;
    int16_t mipLevels = -1;
    int16_t sliceOffset = -1;
    int16_t arraySize = -1;
  private:
    inline bool overlapRange(int16_t xoffset, int16_t xsize, int16_t yoffset, int16_t ysize) const
    {
      return xoffset < yoffset + ysize && yoffset < xoffset + xsize;
    }
  public:
	  SubresourceRange() {}
    SubresourceRange(int16_t mo, int16_t ml, int16_t so, int16_t as)
      : mipOffset(mo)
      , mipLevels(ml)
      , sliceOffset(so)
      , arraySize(as)
    {

    }
    bool overlap(SubresourceRange other) const
    {
      if (mipOffset == SubresourceRange::WholeResource || other.mipOffset == SubresourceRange::WholeResource)
        return true;
      return overlapRange(mipOffset, mipLevels, other.mipOffset, other.mipLevels) // if mips don't overlap, no hope.
        && overlapRange(sliceOffset, arraySize, other.sliceOffset, other.arraySize); // otherwise also arrays need to match.
    }
  };

  namespace RangeMath
  {
    inline SubresourceRange getRightSide(SubresourceRange range, SubresourceRange splitWithThis)
    {
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.sliceOffset - range.sliceOffset), 0));
      return SubresourceRange( range.mipOffset, range.mipLevels, range.sliceOffset, arraySize );
    }

    inline SubresourceRange getLeftSide(SubresourceRange range, SubresourceRange splitWithThis)
    {
      int16_t arraySlice = splitWithThis.sliceOffset + splitWithThis.arraySize;
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>((range.sliceOffset + range.arraySize) - (splitWithThis.sliceOffset + splitWithThis.arraySize)), 0));
      return SubresourceRange(range.mipOffset, range.mipLevels, arraySlice, arraySize );
    }

    inline SubresourceRange getBottomSide(SubresourceRange range, SubresourceRange splitWithThis)
    {
      int16_t miplevels = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.mipOffset - range.mipOffset), 0));
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.sliceOffset + splitWithThis.arraySize - splitWithThis.sliceOffset), 0));
      return SubresourceRange( range.mipOffset, miplevels, splitWithThis.sliceOffset, arraySize );
    }

    inline SubresourceRange getUpSide(SubresourceRange range, SubresourceRange splitWithThis)
    {
      int16_t miplevels = static_cast<int16_t>(std::max(static_cast<int>(range.mipOffset + range.mipLevels - (splitWithThis.mipOffset + splitWithThis.mipLevels)), 0));
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.sliceOffset + splitWithThis.arraySize - splitWithThis.sliceOffset), 0));
      int16_t mipOffset = splitWithThis.mipOffset + splitWithThis.mipLevels;
      return SubresourceRange( mipOffset, miplevels, splitWithThis.sliceOffset, arraySize );
    }

    template <typename Func>
    void getRightSideFunc(const SubresourceRange& range, const SubresourceRange& splitWithThis, Func&& f)
    {
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.sliceOffset - range.sliceOffset), 0));
      if (arraySize > 0 && range.mipLevels > 0)
        f(SubresourceRange( range.mipOffset, range.mipLevels, range.sliceOffset, arraySize ));
    }

    template <typename Func>
    void getLeftSideFunc(const SubresourceRange& range, const SubresourceRange& splitWithThis, Func&& f)
    {
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>((range.sliceOffset + range.arraySize) - (splitWithThis.sliceOffset + splitWithThis.arraySize)), 0));
      if (arraySize > 0 && range.mipLevels > 0)
      {
        int16_t arraySlice = splitWithThis.sliceOffset + splitWithThis.arraySize;
        f(SubresourceRange( range.mipOffset, range.mipLevels, arraySlice, arraySize ));
      }
    }

    template <typename Func>
    void getBottomSideFunc(const SubresourceRange& range, const SubresourceRange& splitWithThis, Func&& f)
    {
      int16_t miplevels = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.mipOffset - range.mipOffset), 0));
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.sliceOffset + splitWithThis.arraySize - splitWithThis.sliceOffset), 0));
      if (arraySize > 0 && miplevels > 0)
      {
        f(SubresourceRange( range.mipOffset, miplevels, splitWithThis.sliceOffset, arraySize ));
      }
    }

    template <typename Func>
    void getUpSideFunc(const SubresourceRange& range, const SubresourceRange& splitWithThis, Func&& f)
    {
      int16_t miplevels = static_cast<int16_t>(std::max(static_cast<int>(range.mipOffset + range.mipLevels - (splitWithThis.mipOffset + splitWithThis.mipLevels)), 0));
      int16_t arraySize = static_cast<int16_t>(std::max(static_cast<int>(splitWithThis.sliceOffset + splitWithThis.arraySize - splitWithThis.sliceOffset), 0));
      if (arraySize > 0 && miplevels > 0)
      {
        int16_t mipOffset = splitWithThis.mipOffset + splitWithThis.mipLevels;
        f(SubresourceRange(mipOffset, miplevels, splitWithThis.sliceOffset, arraySize ));
      }
    }

    inline bool emptyRange(SubresourceRange range)
    {
      return (range.arraySize == 0 || range.mipLevels == 0) ? true : false;
    }
    template <typename Func>
    void difference(const SubresourceRange& range, const SubresourceRange& splitWithThis, Func&& f)
    {
      RangeMath::getRightSideFunc(range, splitWithThis, f);
      RangeMath::getLeftSideFunc(range, splitWithThis, f);
      RangeMath::getBottomSideFunc(range, splitWithThis, f);
      RangeMath::getUpSideFunc(range, splitWithThis, f);
    }
  }

  class SubresourceData
  {
    MemView<uint8_t> m_data;
    size_t m_rowPitch;
    size_t m_slicePitch;
    int3 m_dim;
  public:
    SubresourceData(MemView<uint8_t> data, size_t rowPitch, size_t slicePitch, int3 dim)
      : m_data(data)
      , m_rowPitch(rowPitch)
      , m_slicePitch(slicePitch)
      , m_dim(dim)
    {
    }

    size_t rowPitch()
    {
      return m_rowPitch;
    }

    size_t slicePitch()
    {
      return m_rowPitch;
    }

    uint8_t* data()
    {
      return m_data.data();
    }

    const uint8_t* data() const
    {
      return m_data.data();
    }

    size_t size()
    {
      return m_data.size();
    }

    size_t size() const
    {
      return m_data.size();
    }

    int3 dim() const
    {
      return m_dim;
    }
  };

  inline int3 calculateMipDim(int3 size, unsigned mipOffset)
  {
    int3 dim = size;

    for (auto mip = 1u; mip <= mipOffset; ++mip)
    {
      auto mipDim = int3{ std::max(1, dim.x / 2), std::max(1, dim.y / 2), std::max(1, dim.z / 2) };
      dim = mipDim;
    }
    return dim;
  }

  inline int calculateMaxMipLevels(int3 size)
  {
    int mipLevels = 0;
    while (!(size.x == 1 && size.y == 1 && size.z == 1))
    {
      mipLevels++;
      size = calculateMipDim(size, 1);
    }
    mipLevels++;
    return mipLevels;
  }

  inline int calculatePitch(int3 size, FormatType type)
  {
    int3 dim = size;
    auto pixelSize = formatSizeInfo(type).pixelSize;
    auto rowPitch = pixelSize * dim.x;
    auto slicePitch = rowPitch * dim.y;
    return slicePitch * dim.z;
  }

  class SwapchainDescriptor
  {
  public:
    struct descriptor
    {
      PresentMode mode = PresentMode::Fifo;
      FormatType format = FormatType::Unorm8RGBA;
      int bufferCount = 2;
      Colorspace colorSpace = Colorspace::BT709;

      // if supported
      int frameLatency = 3;
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
    SwapchainDescriptor& frameLatency(int latency)
    {
      desc.frameLatency = latency;
      return *this;
    }
  };

  enum class ResourceShaderType
  {
    Unknown,
    ReadOnly,
    ReadWrite,
    IndexBuffer,
    RenderTarget,
    DepthStencil,
    RaytracingAccelerationStructure
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
    static const unsigned AllMips = unsigned(-1);
    struct descriptor
    {
      std::string     name = "Unnamed Resource";
      FormatType      format = FormatType::Unknown;
      ResourceUsage   usage = ResourceUsage::GpuReadOnly;
      FormatDimension dimension = FormatDimension::Unknown;
      unsigned        stride = 1;
      unsigned        miplevels = 1;
      uint64_t        width = 1;
      unsigned        height = 1;
      unsigned        depth = 1;
      unsigned        arraySize = 1;
      unsigned        msCount = 1;
      bool            index = false;
      bool            indirect = false;
      bool            allowCrossAdapter = false;
      bool            interopt = false;
      bool            allowSimultaneousAccess = false;
      int             hostGPU = 0;

      uint3 size3D() const
      {
        return uint3(width, height, depth);
      }

      uint32_t sizeBytesBuffer() const
      {
        return width * stride;
      }
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

    ResourceDescriptor& setSize(int2 size)
    {
      if (desc.dimension == FormatDimension::Unknown)
        desc.dimension = FormatDimension::Texture2D;
      desc.width = size.x;
      desc.height = size.y;
      return *this;
    }

    ResourceDescriptor& setSize(int3 size)
    {
      if (desc.dimension == FormatDimension::Unknown) {
        desc.dimension = FormatDimension::Texture2D;
        if (size.z > 1)
          desc.dimension = FormatDimension::Texture3D;
      }
      desc.width = size.x;
      desc.height = size.y;
      desc.depth = size.z;
      return *this;
    }

    ResourceDescriptor& setSize(uint2 size)
    {
      if (desc.dimension == FormatDimension::Unknown)
        desc.dimension = FormatDimension::Texture2D;
      desc.width = size.x;
      desc.height = size.y;
      return *this;
    }

    ResourceDescriptor& setSize(uint3 size)
    {
      if (desc.dimension == FormatDimension::Unknown) {
        desc.dimension = FormatDimension::Texture2D;
        if (size.z > 1)
          desc.dimension = FormatDimension::Texture3D;
      }
      desc.width = size.x;
      desc.height = size.y;
      desc.depth = size.z;
      return *this;
    }

    ResourceDescriptor& setElementsCount(uint64_t elements)
    {
      desc.width = elements;
      return *this;
    }

    ResourceDescriptor& setWidth(uint64_t size)
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

    ResourceDescriptor& setMiplevels(unsigned levels = 1)
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
    ResourceDescriptor& allowCrossAdapter(int hostGPU = 0)
    {
      desc.allowCrossAdapter = true;
      desc.hostGPU = hostGPU;
      return *this;
    }
    ResourceDescriptor& interopt(int hostGPU = 0)
    {
      desc.interopt = true;
      desc.hostGPU = hostGPU;
      return *this;
    }
    ResourceDescriptor& allowSimultaneousAccess()
    {
      desc.allowSimultaneousAccess = true;
      return *this;
    }

    int3 size() const
    {
      return int3(desc.width, desc.height, desc.depth);
    }
  };
}