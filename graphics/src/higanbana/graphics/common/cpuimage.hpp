#pragma once

#include "higanbana/graphics/common/resource_descriptor.hpp"
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/global_debug.hpp>

#include <algorithm>

namespace higanbana
{
  /*
  CpuImage is meant to hold image data in tightly packed array with functions to ease access to the data.
  */
  class CpuImage
  {
    vector<uint8_t> m_data;
    /*
    specifies where the subresource info exists in data
    */
    struct sbinfo
    {
      size_t offset;
      size_t rowPitch;
      size_t slicePitch;
      int3 dim;
    };
    vector<sbinfo> m_subresources;
    ResourceDescriptor m_desc;
  public:
    CpuImage(){}
    CpuImage(ResourceDescriptor desc)
      : m_desc(desc)
    {
      auto& d = desc.desc;
      auto formatSize = formatSizeInfo(d.format);
      vector<sbinfo> mips;
      sbinfo info{};
      info.offset = 0;
      info.dim = int3{ static_cast<int>(d.width), static_cast<int>(d.height), static_cast<int>(d.depth) };
      info.rowPitch = formatSize.pixelSize * info.dim.x;
      info.slicePitch = info.rowPitch * info.dim.y;
      mips.push_back(info);
      for (auto mip = 1u; mip < d.miplevels; ++mip)
      {
        auto mipDim = int3{ std::max(1, info.dim.x / 2), std::max(1, info.dim.y / 2), std::max(1, info.dim.z / 2) };
        info.dim = mipDim;
        info.rowPitch = formatSize.pixelSize * info.dim.x;
        info.slicePitch = info.rowPitch * info.dim.y;
        mips.push_back(info);
        HIGAN_LOG("dim %dx%dx%d\n", mipDim.x, mipDim.y, mipDim.z);
      }

      size_t currentOffset = 0;

      for (auto slice = 0u; slice < d.arraySize; ++slice)
      {
        for (auto mip = 0u; mip < d.miplevels; ++mip)
        {
          auto mipInfo = mips[mip];
          mipInfo.offset = currentOffset;
          currentOffset += mipInfo.slicePitch;
          m_subresources.push_back(mipInfo);
        }
      }

      m_data.resize(currentOffset);
    }

    SubresourceData subresource(int mip, int slice)
    {
      auto resourceID = slice * m_desc.desc.miplevels + mip;
      auto info = m_subresources[resourceID];

      MemView<uint8_t> data(&m_data[info.offset], info.slicePitch * info.dim.z);

      return SubresourceData(data, info.rowPitch, info.slicePitch, info.dim);
    }

    const ResourceDescriptor& desc() const
    {
      return m_desc;
    }
  };
}