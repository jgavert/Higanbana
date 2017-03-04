#if defined(FAZE_PLATFORM_WINDOWS)
#include "dx12resources.hpp"
#include "util/formats.hpp"
#include "faze/src/new_gfx/common/graphicssurface.hpp"
#include "view_descriptor.hpp"
#include "core/src/system/bitpacking.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12Device::DX12Device(GpuInfo info, ComPtr<ID3D12Device> device)
      : m_info(info)
      , m_device(device)
      , m_nodeMask(0) // sli/crossfire index
      , m_generics(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024) // lol 1024, right.
      , m_samplers(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16)
      , m_rtvs(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64)
      , m_dsvs(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 16)
    {
      D3D12_COMMAND_QUEUE_DESC desc{};
      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
      desc.NodeMask = m_nodeMask;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_graphicsQueue.ReleaseAndGetAddressOf()));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_dmaQueue.ReleaseAndGetAddressOf()));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_computeQueue.ReleaseAndGetAddressOf()));

      ComPtr<ID3D12Fence> fence;
      m_device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf()));
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
    }

    D3D12_RESOURCE_DESC DX12Device::fillPlacedBufferInfo(ResourceDescriptor descriptor)
    {
      auto& desc = descriptor.desc;
      D3D12_RESOURCE_DESC dxdesc{};

      dxdesc.Width = desc.width * desc.stride;
      dxdesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      dxdesc.Height = 1;
      dxdesc.DepthOrArraySize = 1;
      dxdesc.MipLevels = 1;
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

    D3D12_RESOURCE_DESC DX12Device::fillPlacedTextureInfo(ResourceDescriptor descriptor)
    {
      auto& desc = descriptor.desc;
      D3D12_RESOURCE_DESC dxdesc{};

      dxdesc.Width = desc.width;
      dxdesc.Height = desc.height;
      dxdesc.DepthOrArraySize = static_cast<uint16_t>(desc.arraySize);
      dxdesc.MipLevels = static_cast<uint16_t>(desc.miplevels);
      dxdesc.Alignment = 0; // D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
      dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      dxdesc.Format = formatTodxFormat(desc.format).raw;
      dxdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      dxdesc.SampleDesc.Count = desc.msCount;
      dxdesc.SampleDesc.Quality = 0;
      if (desc.msCount > 1)
      {
        dxdesc.Alignment = 0; // D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
        dxdesc.SampleDesc.Quality = DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN;
      }
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

    std::shared_ptr<prototypes::SwapchainImpl> DX12Device::createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int bufferCount)
    {
      auto natSurface = std::static_pointer_cast<DX12GraphicsSurface>(surface.native());
      RECT rect{};
      BOOL lol = GetClientRect(natSurface->native(), &rect);
      F_ASSERT(lol, "window rect failed ....?");
      auto width = rect.right - rect.left;
      auto height = rect.bottom - rect.top;
      //F_SLOG("DX12", "creating swapchain to %ux%u\n", width, height);
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
      swapChainDesc.Width = width; // I wonder how to get sane sizes in beginning...
      swapChainDesc.Height = height;
      swapChainDesc.Format = formatTodxFormat(format).storage;
      swapChainDesc.Stereo = FALSE;
      swapChainDesc.SampleDesc.Count = 1;
      swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc.BufferCount = static_cast<UINT>(bufferCount); // conviently array can be used to describe bufferCount
      swapChainDesc.Scaling = DXGI_SCALING_NONE; // scaling.. hmm ignore for now
      swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // just use flip_sequential, DXGI_SWAP_EFFECT_FLIP_DISCARD
      swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING interesting
                                                                    // also DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT

      DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullDesc{};
      fullDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // could be something else, like DXGI_MODE_SCALING_STRETCHED
      fullDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE; // no questions about this.
      fullDesc.RefreshRate.Numerator = 60000; // lol 1
      fullDesc.RefreshRate.Denominator = 1001; // lol 2
      fullDesc.Windowed = TRUE; // oh boy, need to toggle this in flight only. fullscreen + breakpoint + single screen == pain.

      ComPtr<IDXGIFactory5> dxgiFactory = nullptr;
      FAZE_CHECK_HR(CreateDXGIFactory2(0, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));


      D3D12Swapchain* swapChain = nullptr;
      FAZE_CHECK_HR(dxgiFactory->CreateSwapChainForHwnd(m_graphicsQueue.Get(), natSurface->native(), &swapChainDesc, &fullDesc, nullptr, (IDXGISwapChain1**)&swapChain));
      auto sc = std::make_shared<DX12Swapchain>(swapChain, *natSurface);
      sc->setBufferMetadata(width, height, bufferCount, format, mode);
      //dxgiFactory->MakeWindowAssociation(natSurface->native(), DXGI_MWA_NO_WINDOW_CHANGES);
      return sc;
    }

    void DX12Device::adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> swapchain, PresentMode mode, FormatType format, int bufferCount)
    {
      auto natSwapchain = std::static_pointer_cast<DX12Swapchain>(swapchain);
      auto& natSurface = natSwapchain->surface();

      RECT rect{};
      BOOL lol = GetClientRect(natSurface.native(), &rect);
      F_ASSERT(lol, "window rect failed ....?");
      auto width = rect.right - rect.left;
      auto height = rect.bottom - rect.top;
      F_SLOG("DX12", "adjusting swapchain to %ux%u\n", width, height);

      format = (format == FormatType::Unknown) ? natSwapchain->getDesc().format : format;
      bufferCount = (bufferCount == -1) ? natSwapchain->getDesc().buffers : bufferCount;
      mode = (mode == PresentMode::Unknown) ? natSwapchain->getDesc().mode : mode;

      /*
      UINT bufferLocations[] = { 1 };
      IUnknown* queues[] = { m_graphicsQueue.Get() };*/

      FAZE_CHECK_HR(natSwapchain->native()->ResizeBuffers(bufferCount, width, height, formatTodxFormat(format).storage, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH/*, bufferLocations, queues*/));

      natSwapchain->setBufferMetadata(width, height, bufferCount, format, mode);
    }

    void DX12Device::destroySwapchain(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);
      native->native()->Release();
    }

    vector<std::shared_ptr<prototypes::TextureImpl>> DX12Device::getSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);

      vector<std::shared_ptr<prototypes::TextureImpl>> textures;
      textures.resize(native->getDesc().buffers);

      for (int i = 0; i < native->getDesc().buffers; ++i)
      {
        ID3D12Resource* renderTarget;
        FAZE_CHECK_HR(native->native()->GetBuffer(i, IID_PPV_ARGS(&renderTarget)));
        textures[i] = std::make_shared<DX12Texture>(renderTarget);
      }
      return textures;
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

      D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
      HeapType type;

      if (desc.desc.dimension == FormatDimension::Buffer)
      {
        resDesc = fillPlacedBufferInfo(desc);
        flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
      }
      else
      {
        resDesc = fillPlacedTextureInfo(desc);
        if (desc.desc.usage == ResourceUsage::RenderTarget
          || desc.desc.usage == ResourceUsage::RenderTargetRW
          || desc.desc.usage == ResourceUsage::DepthStencil
          || desc.desc.usage == ResourceUsage::DepthStencilRW)
        {
          flags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        }
      }

      requirements = m_device->GetResourceAllocationInfo(m_nodeMask, 1, &resDesc);
      reqs.alignment = requirements.Alignment;
      reqs.bytes = requirements.SizeInBytes;

      type = HeapType::Default;
      if (desc.desc.usage == ResourceUsage::Upload)
        type = HeapType::Upload;
      else if (desc.desc.usage == ResourceUsage::Readback)
        type = HeapType::Readback;

      reqs.heapType = packInt64(static_cast<int32_t>(flags), static_cast<int32_t>(type));

      return reqs;
    }

    GpuHeap DX12Device::createHeap(HeapDescriptor heapDesc)
    {
      auto desc = heapDesc.desc;
      D3D12_HEAP_DESC dxdesc{};
      bool smallResource = false;
      if (heapDesc.desc.alignment == D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
      {
        smallResource = true;
        heapDesc.desc.alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      }
      // TODO: properly implement small resources, need to pass these flags around. 
      // textures support small and the same heap cannot contain buffers or rt/ds textures.
      // disabled for now.

      dxdesc.Alignment = heapDesc.desc.alignment;
      dxdesc.SizeInBytes = heapDesc.desc.sizeInBytes;
      dxdesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
      dxdesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
      if (desc.heapType != HeapType::Custom)
      {
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

        if (smallResource)
        {
          dxdesc.Flags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
        }
        if (desc.heapType == HeapType::Upload)
        {
          dxdesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        }
        else if (desc.heapType == HeapType::Readback)
        {
          dxdesc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
        }
      }
      else
      {
        int32_t flags;
        int32_t type;
        unpackInt64(desc.customType, flags, type);

        HeapType hType = static_cast<HeapType>(type);
        if (hType == HeapType::Upload)
        {
          dxdesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        }
        else if (hType == HeapType::Readback)
        {
          dxdesc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
        }
        dxdesc.Flags = static_cast<D3D12_HEAP_FLAGS>(flags);
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

    std::shared_ptr<prototypes::BufferImpl> DX12Device::createBuffer(HeapAllocation allocation, ResourceDescriptor& desc)
    {
      auto native = std::static_pointer_cast<DX12Heap>(allocation.heap.impl);
      auto dxDesc = fillPlacedBufferInfo(desc);
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
      m_device->CreatePlacedResource(native->native(), allocation.allocation.block.offset, &dxDesc, startState, nullptr, IID_PPV_ARGS(&buffer));

      return std::make_shared<DX12Buffer>(buffer);
    }

    void DX12Device::destroyBuffer(std::shared_ptr<prototypes::BufferImpl> buffer)
    {
      auto native = std::static_pointer_cast<DX12Buffer>(buffer);
      native->native()->Release();
    }

    std::shared_ptr<prototypes::BufferViewImpl> DX12Device::createBufferView(
      std::shared_ptr<prototypes::BufferImpl> buffer, ResourceDescriptor& bufferDesc, ShaderViewDescriptor& viewDesc)
    {
      auto native = std::static_pointer_cast<DX12Buffer>(buffer);

      auto desc = bufferDesc.desc;

      FormatType format = viewDesc.m_format;
      if (format == FormatType::Unknown)
        format = desc.format;

      auto descriptor = m_generics.allocate();

      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        D3D12_SHADER_RESOURCE_VIEW_DESC natDesc{};
        natDesc.Format = formatTodxFormat(format).view;
        natDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        natDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        natDesc.Buffer.FirstElement = viewDesc.m_firstElement;
        natDesc.Buffer.NumElements = viewDesc.m_elementCount;
        natDesc.Buffer.StructureByteStride = (format == FormatType::Unknown) ? desc.stride : 0;
        natDesc.Buffer.Flags = (format == FormatType::Raw32) ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
        m_device->CreateShaderResourceView(native->native(), &natDesc, descriptor.cpu);
      }
      else
      {
        D3D12_UNORDERED_ACCESS_VIEW_DESC natDesc{};
        natDesc.Format = formatTodxFormat(format).view;
        natDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        natDesc.Buffer.FirstElement = viewDesc.m_firstElement;
        natDesc.Buffer.NumElements = viewDesc.m_elementCount;
        natDesc.Buffer.StructureByteStride = (format == FormatType::Unknown) ? desc.stride : 0;
        natDesc.Buffer.CounterOffsetInBytes = 0;
        natDesc.Buffer.Flags = (format == FormatType::Raw32) ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
        m_device->CreateUnorderedAccessView(native->native(), nullptr, &natDesc, descriptor.cpu);
      }

      return std::make_shared<DX12BufferView>(descriptor);
    }

    void DX12Device::destroyBufferView(std::shared_ptr<prototypes::BufferViewImpl> view)
    {
      auto native = std::static_pointer_cast<DX12BufferView>(view);
      m_generics.release(native->native());
    }

    std::shared_ptr<prototypes::TextureImpl> DX12Device::createTexture(HeapAllocation allocation, ResourceDescriptor& desc)
    {
      auto native = std::static_pointer_cast<DX12Heap>(allocation.heap.impl);
      auto dxDesc = fillPlacedTextureInfo(desc);
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

      ID3D12Resource* texture;
      m_device->CreatePlacedResource(native->native(), allocation.allocation.block.offset, &dxDesc, startState, nullptr, IID_PPV_ARGS(&texture));

      return std::make_shared<DX12Texture>(texture);
    }

    void DX12Device::destroyTexture(std::shared_ptr<prototypes::TextureImpl> texture)
    {
      auto native = std::static_pointer_cast<DX12Texture>(texture);
      native->native()->Release();
    }

    std::shared_ptr<prototypes::TextureViewImpl> DX12Device::createTextureView(
      std::shared_ptr<prototypes::TextureImpl> texture, ResourceDescriptor& texDesc, ShaderViewDescriptor& viewDesc)
    {
      auto native = std::static_pointer_cast<DX12Texture>(texture);
      auto descriptor = m_generics.allocate();

      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        auto natDesc = dx12::getSRV(texDesc, viewDesc);
        m_device->CreateShaderResourceView(native->native(), &natDesc, descriptor.cpu);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::ReadWrite)
      {
        auto natDesc = dx12::getUAV(texDesc, viewDesc);
        m_device->CreateUnorderedAccessView(native->native(), nullptr, &natDesc, descriptor.cpu);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::RenderTarget)
      {
        auto natDesc = dx12::getRTV(texDesc, viewDesc);
        m_device->CreateRenderTargetView(native->native(), &natDesc, descriptor.cpu);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::DepthStencil)
      {
        auto natDesc = dx12::getDSV(texDesc, viewDesc);
        m_device->CreateDepthStencilView(native->native(), &natDesc, descriptor.cpu);
      }
      return std::make_shared<DX12TextureView>(descriptor);
    }

    void DX12Device::destroyTextureView(std::shared_ptr<prototypes::TextureViewImpl> view)
    {
      auto native = std::static_pointer_cast<DX12TextureView>(view);
      auto descriptor = native->native();
      switch (descriptor.type)
      {
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        {
          m_rtvs.release(native->native());
          break;
        }
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
        {
          m_dsvs.release(native->native());
          break;
        }
        default:
        {
          m_generics.release(native->native());
          break;
        }
      }
    }
  }
}
#endif