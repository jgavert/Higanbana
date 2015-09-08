#pragma once
#include "ComPtr.hpp"
#include "Descriptors/Descriptors.hpp"
#include <d3d12.h>

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC&, D3D12_HEAP_PROPERTIES&)
{}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  ArraySize size, Args... args)
{
  desc.DepthOrArraySize = size.size;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  Dimension dim, Args... args)
{
  desc.Width *= dim.width;
  desc.Height *= dim.height;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename T, typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES&heapProp,
  Format<T> format, Args... args)
{
  desc.Format = FormatToDXGIFormat[format.format];
  if (FormatType::Unknown == format.format) // ONLY FOR BUFFERS !?!?!?!?
  {
    desc.DepthOrArraySize = format.stride;
  }
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  GpuShared shared, Args... args)
{
  if (shared.shared)
  {
    heapProp.VisibleNodeMask = 1;
    heapProp.CreationNodeMask = 1;
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  ResType type, Args... args)
{
  switch (type)
  {
  case ResType::Upload:
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    break;
  case ResType::Gpu:
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    break;
  case ResType::Readback:
    heapProp.Type = D3D12_HEAP_TYPE_READBACK;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    break;
  default:
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    break;
  }
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  Multisampling sampling, Args... args)
{
  desc.SampleDesc.Count = sampling.count;
  desc.SampleDesc.Quality = sampling.quality;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  MipLevel mip, Args... args)
{
  desc.MipLevels = mip.level;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}



