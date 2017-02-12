#if defined(PLATFORM_WINDOWS)
#include "dx12resources.hpp"
#include "util/formats.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12Device::DX12Device(GpuInfo info, ComPtr<ID3D12Device> device)
      : m_info(info)
      , m_device(device)
      , m_nodeMask(1 << m_info.id)
    {
      D3D12_COMMAND_QUEUE_DESC desc{};
      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
      desc.NodeMask = m_nodeMask;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_graphicsQueue.GetAddressOf()));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_dmaQueue.GetAddressOf()));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_computeQueue.GetAddressOf()));

      ComPtr<ID3D12Fence> fence;
      m_device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
      m_deviceFence = DX12Fence(fence);
      /*
      D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS data{};
      for (int i = 0; i < 64; ++i)
      {
        data.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        data.NumQualityLevels = 1;
        data.SampleCount = i;
        data.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &data, sizeof(data));
        if (hr == S_OK)
        {
          F_SLOG("DX12", "Supported msaa mode with SampleCount %d\n", i);
        }
      }*/
    }

    DX12Device::~DX12Device()
    {
      waitGpuIdle();
      /*
      ComPtr<ID3D12DebugDevice> debugInterface;
      m_device.As(&debugInterface);
      debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
      */
    }

    D3D12_RESOURCE_DESC DX12Device::fillBufferInfo(ResourceDescriptor descriptor)
    {
      auto& desc = descriptor.desc;
      D3D12_RESOURCE_DESC dxdesc{};

      dxdesc.Width = desc.width*desc.stride;
      dxdesc.Height = 1;
      dxdesc.DepthOrArraySize = 1;
      dxdesc.MipLevels = 1;
      dxdesc.Alignment = 0;
      dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      dxdesc.Format = DXGI_FORMAT_UNKNOWN;
      dxdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      dxdesc.SampleDesc.Count = 1;
      dxdesc.SampleDesc.Quality = 0;
      dxdesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      switch (desc.usage)
      {
      case ResourceUsage::GpuReadOnly:
      {
        break;
      }
      case ResourceUsage::GpuRW:
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        break;
      }
      default:
        break;
      }

      return dxdesc;
    }

    D3D12_RESOURCE_DESC DX12Device::fillTextureInfo(ResourceDescriptor descriptor)
    {
      auto& desc = descriptor.desc;
      D3D12_RESOURCE_DESC dxdesc{};

      dxdesc.Width = desc.width;
      dxdesc.Height = desc.height;
      dxdesc.DepthOrArraySize = static_cast<uint16_t>(desc.arraySize);
      dxdesc.MipLevels = static_cast<uint16_t>(desc.miplevels);
      dxdesc.Alignment = 0;
      dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      dxdesc.Format = formatTodxFormat(desc.format).raw;
      dxdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      dxdesc.SampleDesc.Count = desc.msCount;
      dxdesc.SampleDesc.Quality = desc.msQuality;
      dxdesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      switch (desc.dimension)
      {
      case FormatDimension::Texture1D:
        dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
      case FormatDimension::Texture2D:
        dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
      case FormatDimension::Texture3D:
        dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        dxdesc.DepthOrArraySize = static_cast<uint16_t>(desc.depth);
        break;
      case FormatDimension::TextureCube:
        dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dxdesc.DepthOrArraySize = static_cast<uint16_t>(desc.arraySize * 6);
      default:
        break;
      }

      switch (desc.usage)
      {
      case ResourceUsage::GpuReadOnly:
      {
        break;
      }
      case ResourceUsage::GpuRW:
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        break;
      }
      case ResourceUsage::RenderTarget:
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        break;
      }
      case ResourceUsage::RenderTargetRW:
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        break;
      }
      case ResourceUsage::DepthStencil:
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        F_ASSERT(desc.miplevels == 1, "DepthStencil doesn't support mips");
        break;
      }
      case ResourceUsage::DepthStencilRW:
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        F_ASSERT(desc.miplevels == 1, "DepthStencil doesn't support mips");
        break;
      }
      default:
        break;
      }


      return dxdesc;
    }

    void DX12Device::waitGpuIdle()
    {
      m_graphicsQueue->Signal(m_deviceFence.fence.Get(), m_deviceFence.start());
      m_deviceFence.waitTillReady();
    }

    MemoryRequirements DX12Device::getReqs(ResourceDescriptor desc)
    {
      MemoryRequirements reqs{};
      D3D12_RESOURCE_ALLOCATION_INFO requirements;
      D3D12_RESOURCE_DESC resDesc;
      if (desc.desc.dimension == FormatDimension::Buffer)
      {
        resDesc = fillBufferInfo(desc);
      }
      else
      {
        resDesc = fillTextureInfo(desc);
      }
      requirements = m_device->GetResourceAllocationInfo(m_nodeMask, 1, &resDesc);
      reqs.alignment = requirements.Alignment;
      reqs.bytes = requirements.SizeInBytes;
      return reqs;
    }

    GpuHeap DX12Device::createHeap(HeapDescriptor heapDesc)
    {
      auto desc = heapDesc.desc;
      D3D12_HEAP_DESC dxdesc{};

      dxdesc.Alignment = heapDesc.desc.alignment;
      dxdesc.SizeInBytes = heapDesc.desc.sizeInBytes;
      dxdesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
      dxdesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
      if (desc.onlyBuffers)
      {
        dxdesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
      }
      else if (desc.onlyNonRtDsTextures)
      {
        dxdesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
      }
      else if (desc.onlyRtDsTextures)
      {
        dxdesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
      }
      if (desc.heapType == HeapType::Upload)
      {
        dxdesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
      }
      else if (desc.heapType == HeapType::Readback)
      {
        dxdesc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
      }

      ID3D12Heap* heap;
      m_device->CreateHeap(&dxdesc, IID_PPV_ARGS(&heap));
      return GpuHeap(std::make_shared<DX12Heap>(heap), std::move(heapDesc));
    }

    void DX12Device::destroyHeap(GpuHeap heap)
    {
      auto native = std::static_pointer_cast<DX12Heap>(heap.impl);
      native->native()->Release();
    }

    void DX12Device::createBuffer(GpuHeap heap, size_t offset, ResourceDescriptor desc)
    {
      auto native = std::static_pointer_cast<DX12Heap>(heap.impl);
      auto dxDesc = fillBufferInfo(desc);
      D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATE_COMMON;
      switch (desc.desc.usage)
      {
      case ResourceUsage::Upload:
      {
        startState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;
      }
      case ResourceUsage::Readback:
      {
        startState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;
      }
      default:
        break;
      }

      ID3D12Resource* buffer;
      m_device->CreatePlacedResource(native->native(), offset, &dxDesc, startState, nullptr, IID_PPV_ARGS(&buffer));
      buffer->Release();
      // requires stuff...
    }

    void DX12Device::createBufferView(ShaderViewDescriptor )
    {

    }
  }
}
#endif