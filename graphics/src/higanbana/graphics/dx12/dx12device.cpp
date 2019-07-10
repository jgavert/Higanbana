#include "higanbana/graphics/dx12/dx12device.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/util/formats.hpp"
#include "higanbana/graphics/common/graphicssurface.hpp"
#include "higanbana/graphics/dx12/view_descriptor.hpp"
#include "higanbana/graphics/definitions.hpp"
#include "higanbana/graphics/dx12/util/pipeline_helpers.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include <higanbana/core/system/bitpacking.hpp>
#include <higanbana/core/global_debug.hpp>

#include <DXGIDebug.h>

namespace higanbana
{
  namespace backend
  {
    DX12Device::DX12Device(GpuInfo info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory4> factory, FileSystem& fs, bool debugLayer)
      : m_info(info)
      , m_debugLayer(debugLayer)
      , m_device(device)
      , m_factory(factory)
      , m_fs(fs)
      , m_shaders(fs, std::shared_ptr<ShaderCompiler>(new DXCompiler(fs, "/shaders/")), "shaders", "shaders/bin", ShaderBinaryType::DXIL)
      , m_nodeMask(0) // sli/crossfire index
      , m_generics(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024) // lol 1024, right.
      //, m_samplers(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16)
      , m_rtvs(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64)
      , m_dsvs(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 16)
      , m_constantsUpload(std::make_shared<DX12UploadHeap>(device.Get(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 64, 512)) // we have room for 64*512 drawcalls worth of constants.
      , m_dynamicUpload(std::make_shared<DX12UploadHeap>(device.Get(), 256 * 256, 1024 * 1)) // we have room 64 / 4megs of dynamic buffers
      , m_dynamicGpuDescriptors(std::make_shared<DX12DynamicDescriptorHeap>(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, 480))
      //, m_trash(std::make_shared<Garbage>())
      //, m_seqTracker(std::make_shared<SequenceTracker>())
    {
      if (m_debugLayer)
      {
        DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_debug));
        //m_debug->EnableLeakTrackingForThread();
      }
      D3D12_COMMAND_QUEUE_DESC desc{};
      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
      desc.NodeMask = m_nodeMask;
      HIGANBANA_CHECK_HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_graphicsQueue.ReleaseAndGetAddressOf())));

      {
        auto wstr = s2ws("GraphicsQueue");
        m_graphicsQueue->SetName(wstr.c_str());
      }

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      HIGANBANA_CHECK_HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_dmaQueue.ReleaseAndGetAddressOf())));
      {
        auto wstr = s2ws("dmaQueue");
        m_dmaQueue->SetName(wstr.c_str());
      }

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      HIGANBANA_CHECK_HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_computeQueue.ReleaseAndGetAddressOf())));
      {
        auto wstr = s2ws("computeQueue");
        m_computeQueue->SetName(wstr.c_str());
      }

      m_deviceFence = createNativeFence();

      m_queryHeapPool = Rabbitpool2<DX12QueryHeap>([&]()
      {
        return createGraphicsQueryHeap(128); // maybe 10 megs of readback?
      });

      m_computeQueryHeapPool = Rabbitpool2<DX12QueryHeap>([&]()
      {
        return createComputeQueryHeap(128); // maybe 10 megs of readback?
      });

      m_readbackPool = Rabbitpool2<DX12ReadbackHeap>([&]()
      {
        return createReadback(256 * 10, 1024 * 4); // maybe 10 megs of readback?
      });

      m_copyListPool = Rabbitpool2<DX12CommandBuffer>([&]()
      {
        return createList(D3D12_COMMAND_LIST_TYPE_COPY);
      });

      m_computeListPool = Rabbitpool2<DX12CommandBuffer>([&]()
      {
        return createList(D3D12_COMMAND_LIST_TYPE_COMPUTE);
      });

      m_graphicsListPool = Rabbitpool2<DX12CommandBuffer>([&]()
      {
        return createList(D3D12_COMMAND_LIST_TYPE_DIRECT);
      });

      m_fencePool = Rabbitpool2<DX12Fence>([&]()
      {
        return createNativeFence();
      });
      m_semaPool = Rabbitpool2<DX12Semaphore>([&]()
      {
        return createNativeSemaphore();
      });

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
          HIGAN_SLOG("DX12", "Supported msaa mode with SampleCount %d\n", i);
        }
      }*/

      {
        m_nullBufferUAV = m_generics.allocate();
        D3D12_UNORDERED_ACCESS_VIEW_DESC nulldesc = {};
        nulldesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nulldesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        nulldesc.Buffer.FirstElement = 0;
        nulldesc.Buffer.NumElements = 1;
        nulldesc.Buffer.StructureByteStride = 0;
        nulldesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        m_device->CreateUnorderedAccessView(
          nullptr, nullptr,
          &nulldesc,
          m_nullBufferUAV.cpu);
      }
      {
        m_nullBufferSRV = m_generics.allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC nulldesc = {};
        nulldesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nulldesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        nulldesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nulldesc.Buffer.FirstElement = 0;
        nulldesc.Buffer.NumElements = 1;
        nulldesc.Buffer.StructureByteStride = 0;
        nulldesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        m_device->CreateShaderResourceView(
          nullptr,
          &nulldesc,
          m_nullBufferSRV.cpu);
      }
    }

    DX12Device::~DX12Device()
    {
      waitGpuIdle();
      if (m_debugLayer)
      {
        m_debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
      }
    }

    DX12Resources& DX12Device::allResources()
    {
      return m_allRes;
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

      if (desc.allowCrossAdapter)
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
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
        HIGAN_ASSERT(desc.miplevels == 1, "DepthStencil doesn't support mips");
        break;
      }
      case ResourceUsage::DepthStencilRW:
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        HIGAN_ASSERT(desc.miplevels == 1, "DepthStencil doesn't support mips");
        break;
      }
      default:
        break;
      }

      if (desc.allowCrossAdapter)
      {
        dxdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
        dxdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      }

      return dxdesc;
    }

    std::shared_ptr<prototypes::SwapchainImpl> DX12Device::createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor)
    {
      auto natSurface = std::static_pointer_cast<DX12GraphicsSurface>(surface.native());
      RECT rect{};
      BOOL lol = GetClientRect(natSurface->native(), &rect);
      HIGAN_ASSERT(lol, "window rect failed ....?");
      auto width = rect.right - rect.left;
      auto height = rect.bottom - rect.top;
      //HIGAN_SLOG("DX12", "creating swapchain to %ux%u\n", width, height);
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
      swapChainDesc.Width = width; // I wonder how to get sane sizes in beginning...
      swapChainDesc.Height = height;
      swapChainDesc.Format = formatTodxFormat(descriptor.desc.format).storage;
      swapChainDesc.Stereo = FALSE;
      swapChainDesc.SampleDesc.Count = 1;
      swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc.BufferCount = static_cast<UINT>(descriptor.desc.bufferCount); // conviently array can be used to describe bufferCount
      swapChainDesc.Scaling = DXGI_SCALING_NONE; // scaling.. hmm ignore for now
      swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // just use flip_sequential, DXGI_SWAP_EFFECT_FLIP_DISCARD
      swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING interesting
                                                                    // also DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT

      /*
      DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullDesc{};
      fullDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // could be something else, like DXGI_MODE_SCALING_STRETCHED
      fullDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE; // no questions about this.
      fullDesc.RefreshRate.Numerator = 60000; // lol 1
      fullDesc.RefreshRate.Denominator = 1001; // lol 2
      fullDesc.Windowed = TRUE; // oh boy, need to toggle this in flight only. fullscreen + breakpoint + single screen == pain.
      */

      ComPtr<D3D12Swapchain> swapChain = nullptr;
      ComPtr<IDXGISwapChain1> base_chain;
      HIGANBANA_CHECK_HR(m_factory->CreateSwapChainForHwnd(m_graphicsQueue.Get(), natSurface->native(), &swapChainDesc, nullptr, nullptr, &base_chain));
      HIGANBANA_CHECK_HR(base_chain.As(&swapChain));
      HIGANBANA_CHECK_HR(swapChain->SetMaximumFrameLatency(descriptor.desc.frameLatency));

      std::shared_ptr<DX12Swapchain> sc;
      HANDLE thingy = swapChain->GetFrameLatencyWaitableObject();
      sc = std::make_shared<DX12Swapchain>(swapChain, *natSurface, thingy);

      sc->setBufferMetadata(width, height, descriptor);
      m_factory->MakeWindowAssociation(natSurface->native(), DXGI_MWA_NO_WINDOW_CHANGES); // if using alt+enter, we would still need to call ResizeBuffers

#if !defined(HIGANBANA_NSIGHT_COMPATIBILITY)
      ensureSwapchainColorspace(sc, descriptor);
#endif
      return sc;
    }

    void DX12Device::adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> swapchain, SwapchainDescriptor d)
    {
      auto natSwapchain = std::static_pointer_cast<DX12Swapchain>(swapchain);
      auto& natSurface = natSwapchain->surface();

      RECT rect{};
      BOOL lol = GetClientRect(natSurface.native(), &rect);
      HIGAN_ASSERT(lol, "window rect failed ....?");
      auto width = rect.right - rect.left;
      auto height = rect.bottom - rect.top;
      //HIGAN_SLOG("DX12", "adjusting swapchain to %ux%u\n", width, height);

      // clean old
      natSwapchain->setSwapchain(nullptr);

      // make new one
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
      swapChainDesc.Width = width; // I wonder how to get sane sizes in beginning...
      swapChainDesc.Height = height;
      swapChainDesc.Format = formatTodxFormat(d.desc.format).storage;
      swapChainDesc.Stereo = FALSE;
      swapChainDesc.SampleDesc.Count = 1;
      swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc.BufferCount = static_cast<UINT>(d.desc.bufferCount); // conviently array can be used to describe bufferCount
      swapChainDesc.Scaling = DXGI_SCALING_NONE; // scaling.. hmm ignore for now
      swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // just use flip_sequential, DXGI_SWAP_EFFECT_FLIP_DISCARD
      swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

      ComPtr<D3D12Swapchain> swapChain = nullptr;
      ComPtr<IDXGISwapChain1> base_chain;
      HIGANBANA_CHECK_HR(m_factory->CreateSwapChainForHwnd(m_graphicsQueue.Get(), natSwapchain->surface().native(), &swapChainDesc, nullptr, nullptr, &base_chain));
      HIGANBANA_CHECK_HR(base_chain.As(&swapChain));
      HIGANBANA_CHECK_HR(swapChain->SetMaximumFrameLatency(d.desc.frameLatency));

      natSwapchain->setSwapchain(swapChain);

      HANDLE thingy = natSwapchain->native()->GetFrameLatencyWaitableObject();
      natSwapchain->setFrameLatencyObj(thingy);

      m_factory->MakeWindowAssociation(natSwapchain->surface().native(), DXGI_MWA_NO_WINDOW_CHANGES); // if using alt+enter, we would still need to call ResizeBuffers

      natSwapchain->setBufferMetadata(width, height, d);

#if !defined(HIGANBANA_NSIGHT_COMPATIBILITY)
      ensureSwapchainColorspace(natSwapchain, d);
#endif
    }

    void DX12Device::ensureSwapchainColorspace(std::shared_ptr<DX12Swapchain> sc, SwapchainDescriptor& descriptor)
    {
      if (!m_factory->IsCurrent())
      {
        HIGANBANA_CHECK_HR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_factory)));
      }

      // Get information about the display we are presenting to.
      ComPtr<IDXGIOutput> output;
      ComPtr<IDXGIOutput6> output6;

      if (SUCCEEDED(sc->native()->GetContainingOutput(&output)))
      {
        if (SUCCEEDED(output.As(&output6)))
        {
          DXGI_OUTPUT_DESC1 outputDesc;
          HIGANBANA_CHECK_HR(output6->GetDesc1(&outputDesc));

          sc->setHDR(outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
        }
      }

      auto enableST2084 = sc->HDRSupport() && (descriptor.desc.colorSpace == Colorspace::BT2020);
      DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
      auto bitdepth = formatBitDepth(sc->getDesc().descriptor.desc.format);
      switch (bitdepth)
      {
      case 8:
        sc->setDisplayCurve(DisplayCurve::sRGB);
        break;
      case 10:
        colorSpace = enableST2084 ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
        sc->setDisplayCurve(enableST2084 ? DisplayCurve::ST2084 : DisplayCurve::sRGB);
        break;
      case 16:
        colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
        sc->setDisplayCurve(DisplayCurve::None);
        break;
      default:
        break;
      }

      //if (colorSpace != sc->nativeColorspace())
      {
        UINT colorSpaceSupport = 0;
        if (SUCCEEDED(sc->native()->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)) &&
          ((colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        {
          HIGANBANA_CHECK_HR(sc->native()->SetColorSpace1(colorSpace));
          sc->setNativeColorspace(colorSpace);
        }
      }
    }

    int DX12Device::fetchSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> swapchain, vector<ResourceHandle>& handles)
    {
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);

      vector<std::shared_ptr<prototypes::TextureImpl>> textures;
      textures.resize(native->getDesc().descriptor.desc.bufferCount);

      DX12ResourceState state{ 0 };
      state.commonStateOptimisation = false;
      state.flags.emplace_back(D3D12_RESOURCE_STATE_COMMON);


      for (int i = 0; i < native->getDesc().descriptor.desc.bufferCount; ++i)
      {
        ID3D12Resource* renderTarget;
        HIGANBANA_CHECK_HR(native->native()->GetBuffer(i, IID_PPV_ARGS(&renderTarget)));
        auto desc = swapchain->desc().setName("renderTarget_" + std::to_string(i) + "_id_" + std::to_string(handles[i].id));
        auto wstr = s2ws(desc.desc.name);
        renderTarget->SetName(wstr.c_str());
        
        m_allRes.tex[handles[i]] = DX12Texture(renderTarget, desc, 1);
      }
      return static_cast<int>(textures.size());
    }

    int DX12Device::tryAcquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
        auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);
        if (!native->checkFrameLatency())
            return -1;
        int index = native->native()->GetCurrentBackBufferIndex();
        native->setBackbufferIndex(index);
        return index;
    }

    int DX12Device::acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);
      native->waitForFrameLatency();
      int index = native->native()->GetCurrentBackBufferIndex();
      native->setBackbufferIndex(index);
      return index;
    }

    // should be about ready
    void DX12Device::releaseHandle(ResourceHandle handle)
    {
      switch(handle.type)
      {
        case ResourceType::Buffer:
        {
          m_allRes.buf[handle].native()->Release();
          m_allRes.buf[handle] = DX12Buffer();
          break;
        }
        case ResourceType::Texture:
        {
          m_allRes.tex[handle].native()->Release();
          m_allRes.tex[handle] = DX12Texture();
          break;
        }
        case ResourceType::Pipeline:
        {
          m_allRes.pipelines[handle] = DX12Pipeline();
          break;
        }
        case ResourceType::DynamicBuffer:
        {
          //m_dynamicUpload->release(dynBuf);
          //m_allRes.[handle] = DX12DynamicBufferView();
          HIGAN_ASSERT(false, "unhandled type released");
          break;
        }
        case ResourceType::MemoryHeap:
        {
          m_allRes.heaps[handle] = DX12Heap();
          break;
        }
        case ResourceType::Renderpass:
        {
          break; // shrug, what renderpass
        }
        default:
        {
          HIGAN_ASSERT(false, "unhandled type released");
          break;
        }
      }
    }

    void DX12Device::releaseViewHandle(ViewResourceHandle handle)
    {
      switch(handle.type)
      {
        case ViewResourceType::BufferIBV:
        {
          m_generics.release(m_allRes.bufIBV[handle].native());
          m_allRes.bufIBV[handle] = DX12BufferView();
          break;
        }
        case ViewResourceType::BufferSRV:
        {
          m_generics.release(m_allRes.bufSRV[handle].native());
          m_allRes.bufSRV[handle] = DX12BufferView();
          break;
        }
        case ViewResourceType::BufferUAV:
        {
          m_generics.release(m_allRes.bufUAV[handle].native());
          m_allRes.bufUAV[handle] = DX12BufferView();
          break;
        }
        case ViewResourceType::TextureSRV:
        {
          m_generics.release(m_allRes.texSRV[handle].native());
          m_allRes.texSRV[handle] = DX12TextureView();
          break;
        }
        case ViewResourceType::TextureUAV:
        {
          m_generics.release(m_allRes.texUAV[handle].native());
          m_allRes.texUAV[handle] = DX12TextureView();
          break;
        }
        case ViewResourceType::TextureRTV:
        {
          m_rtvs.release(m_allRes.texRTV[handle].native());
          m_allRes.texRTV[handle] = DX12TextureView();
          break;
        }
        case ViewResourceType::TextureDSV:
        {
          m_dsvs.release(m_allRes.texDSV[handle].native());
          m_allRes.texDSV[handle] = DX12TextureView();
          break;
        }
        case ViewResourceType::DynamicBufferSRV:
        {
          auto& dynBuf = m_allRes.dynSRV[handle];
          m_dynamicUpload->release(dynBuf.block);
          if (dynBuf.hasDescriptor()) {
            m_generics.release(dynBuf.resource);
          }
          m_allRes.dynSRV[handle] = DX12DynamicBufferView();
          break;
        }
        default:
        {
          HIGAN_ASSERT(false, "unhandled type released");
          break;
        }
      }
    }

    void DX12Device::collectTrash()
    {
      if (m_debugLayer)
        m_debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
    }

    void DX12Device::waitGpuIdle()
    {
      m_graphicsQueue->Signal(m_deviceFence.fence.Get(), m_deviceFence.start());
      m_deviceFence.waitTillReady();
      m_computeQueue->Signal(m_deviceFence.fence.Get(), m_deviceFence.start());
      m_deviceFence.waitTillReady();
      m_dmaQueue->Signal(m_deviceFence.fence.Get(), m_deviceFence.start());
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

      // see https://msdn.microsoft.com/en-us/library/windows/desktop/mt186623(v=vs.85).aspx
      // overrides bunch of requirements.
      if (desc.desc.allowCrossAdapter)
      {
        flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | D3D12_HEAP_FLAG_SHARED | D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER;
      }
      if (desc.desc.interopt)
      {
        flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | D3D12_HEAP_FLAG_SHARED; 
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

    void DX12Device::createRenderpass(ResourceHandle handle)
    {
      return;
    }

    void DX12Device::createPipeline(ResourceHandle handle, GraphicsPipelineDescriptor desc)
    {
      m_allRes.pipelines[handle] = DX12Pipeline();
      m_allRes.pipelines[handle].m_gfxDesc = desc;
    }

    void DX12Device::createPipeline(ResourceHandle handle, ComputePipelineDescriptor desc)
    {
      m_allRes.pipelines[handle] = DX12Pipeline();
      m_allRes.pipelines[handle].m_computeDesc = desc;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC DX12Device::getDesc(GraphicsPipelineDescriptor::Desc& d, gfxpacket::RenderPassBegin& subpass)
    {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
      {
        // BlendState
        desc.BlendState.IndependentBlendEnable = d.blendDesc.desc.independentBlendEnable;
        desc.BlendState.AlphaToCoverageEnable = d.blendDesc.desc.alphaToCoverageEnable;
        auto logicEnable = d.blendDesc.desc.logicOpEnabled;
        auto logicOp = convertLogicOp(d.blendDesc.desc.logicOp);
        for (int i = 0; i < 8; ++i)
        {
          auto& rtb = d.blendDesc.desc.renderTarget[i].desc;
          desc.BlendState.RenderTarget[i].BlendEnable = rtb.blendEnable;
          desc.BlendState.RenderTarget[i].LogicOpEnable = logicEnable;
          desc.BlendState.RenderTarget[i].SrcBlend = convertBlend(rtb.srcBlend);
          desc.BlendState.RenderTarget[i].DestBlend = convertBlend(rtb.destBlend);
          desc.BlendState.RenderTarget[i].BlendOp = convertBlendOp(rtb.blendOp);
          desc.BlendState.RenderTarget[i].SrcBlendAlpha = convertBlend(rtb.srcBlendAlpha);
          desc.BlendState.RenderTarget[i].DestBlendAlpha = convertBlend(rtb.destBlendAlpha);
          desc.BlendState.RenderTarget[i].BlendOpAlpha = convertBlendOp(rtb.blendOpAlpha);
          desc.BlendState.RenderTarget[i].LogicOp = logicOp;
          desc.BlendState.RenderTarget[i].RenderTargetWriteMask = convertColorWriteEnable(rtb.colorWriteEnable);
        }
      }
      {
        // Rasterization
        auto& rd = d.rasterDesc.desc;
        desc.RasterizerState.FillMode = convertFillMode(rd.fill);
        desc.RasterizerState.CullMode = convertCullMode(rd.cull);
        desc.RasterizerState.FrontCounterClockwise = rd.frontCounterClockwise;
        desc.RasterizerState.DepthBias = rd.depthBias;
        desc.RasterizerState.DepthBiasClamp = rd.depthBiasClamp;
        desc.RasterizerState.SlopeScaledDepthBias = rd.slopeScaledDepthBias;
        desc.RasterizerState.DepthClipEnable = rd.depthClipEnable;
        desc.RasterizerState.MultisampleEnable = rd.multisampleEnable;
        desc.RasterizerState.AntialiasedLineEnable = rd.antialiasedLineEnable;
        desc.RasterizerState.ForcedSampleCount = rd.forcedSampleCount;
        desc.RasterizerState.ConservativeRaster = convertConservativeRasterization(rd.conservativeRaster);
      }
      {
        // DepthStencil
        auto& dss = d.dsdesc.desc;
        desc.DepthStencilState.DepthEnable = dss.depthEnable;
        desc.DepthStencilState.DepthWriteMask = convertDepthWriteMask(dss.depthWriteMask);
        desc.DepthStencilState.DepthFunc = convertComparisonFunc(dss.depthFunc);
        desc.DepthStencilState.StencilEnable = dss.stencilEnable;
        desc.DepthStencilState.StencilReadMask = dss.stencilReadMask;
        desc.DepthStencilState.StencilWriteMask = dss.stencilWriteMask;
        desc.DepthStencilState.FrontFace.StencilFailOp = convertStencilOp(dss.frontFace.desc.failOp);
        desc.DepthStencilState.FrontFace.StencilDepthFailOp = convertStencilOp(dss.frontFace.desc.depthFailOp);
        desc.DepthStencilState.FrontFace.StencilPassOp = convertStencilOp(dss.frontFace.desc.passOp);
        desc.DepthStencilState.FrontFace.StencilFunc = convertComparisonFunc(dss.frontFace.desc.stencilFunc);
        desc.DepthStencilState.BackFace.StencilFailOp = convertStencilOp(dss.backFace.desc.failOp);
        desc.DepthStencilState.BackFace.StencilDepthFailOp = convertStencilOp(dss.backFace.desc.depthFailOp);
        desc.DepthStencilState.BackFace.StencilPassOp = convertStencilOp(dss.backFace.desc.passOp);
        desc.DepthStencilState.BackFace.StencilFunc = convertComparisonFunc(dss.backFace.desc.stencilFunc);
      }
      desc.PrimitiveTopologyType = convertPrimitiveTopologyType(d.primitiveTopology);
      desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
      // subpass things here
      auto rtvs = subpass.rtvs.convertToMemView();
      desc.NumRenderTargets = static_cast<unsigned>(rtvs.size());
      for (size_t i = 0; i < rtvs.size(); ++i)
      {
        auto rawFormat = m_allRes.texRTV[rtvs[i]].viewFormat();
        desc.RTVFormats[i] = rawFormat;
      }
      
      desc.DSVFormat = (subpass.dsv.id == ViewResourceHandle::InvalidViewId) ? DXGI_FORMAT_UNKNOWN : m_allRes.texDSV[subpass.dsv].viewFormat();
      for (size_t i = rtvs.size(); i < 8; ++i)
      {
        desc.RTVFormats[i] = formatTodxFormat(d.rtvFormats[i]).view;
      }
      desc.SampleMask = UINT_MAX;
      /*
      desc.NumRenderTargets = d.numRenderTargets;
      for (int i = 0; i < 8; ++i)
      {
        desc.RTVFormats[i] = formatTodxFormat(d.rtvFormats[i]).view;
      }
      desc.DSVFormat = formatTodxFormat(d.dsvFormat).view;
      */
      desc.SampleDesc.Count = d.sampleCount;
      desc.SampleDesc.Quality = d.sampleCount > 1 ? DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN : 0;

      return desc;
    }

    std::optional<DX12OldPipeline> DX12Device::updatePipeline(ResourceHandle pipeline, gfxpacket::RenderPassBegin& renderpass)
    {
      std::optional<DX12OldPipeline> oldPipe;

      auto& vp = m_allRes.pipelines[pipeline];
      if (vp.m_hasPipeline && !vp.needsUpdating())
        return oldPipe;

      auto desc = getDesc(vp.m_gfxDesc.desc, renderpass);

      GraphicsPipelineDescriptor::Desc& d = vp.m_gfxDesc.desc;
      vector<MemoryBlob> blobs;
      if (!d.vertexShaderPath.empty())
      {
        auto shader = m_shaders.shader(ShaderCreateInfo(d.vertexShaderPath, ShaderType::Vertex, d.layout));
        blobs.emplace_back(shader);
        desc.VS.BytecodeLength = blobs.back().size();
        desc.VS.pShaderBytecode = blobs.back().data();

        if (vp.vs.empty())
        {
          vp.vs = m_shaders.watch(d.vertexShaderPath, ShaderType::Vertex);
        }
        vp.vs.react();
      }

      if (!d.hullShaderPath.empty())
      {
        auto shader = m_shaders.shader(ShaderCreateInfo(d.hullShaderPath, ShaderType::TessControl, d.layout));
        blobs.emplace_back(shader);
        desc.HS.BytecodeLength = blobs.back().size();
        desc.HS.pShaderBytecode = blobs.back().data();

        if (vp.hs.empty())
        {
          vp.hs = m_shaders.watch(d.hullShaderPath, ShaderType::TessControl);
        }
        vp.hs.react();
      }

      if (!d.domainShaderPath.empty())
      {
        auto shader = m_shaders.shader(ShaderCreateInfo(d.domainShaderPath, ShaderType::TessEvaluation, d.layout));
        blobs.emplace_back(shader);
        desc.DS.BytecodeLength = blobs.back().size();
        desc.DS.pShaderBytecode = blobs.back().data();
        if (vp.ds.empty())
        {
          vp.ds = m_shaders.watch(d.domainShaderPath, ShaderType::TessEvaluation);
        }
        vp.ds.react();
      }

      if (!d.geometryShaderPath.empty())
      {
        auto shader = m_shaders.shader(ShaderCreateInfo(d.geometryShaderPath, ShaderType::Geometry, d.layout));
        blobs.emplace_back(shader);
        desc.GS.BytecodeLength = blobs.back().size();
        desc.GS.pShaderBytecode = blobs.back().data();
        if (vp.gs.empty())
        {
          vp.gs = m_shaders.watch(d.geometryShaderPath, ShaderType::Geometry);
        }
        vp.gs.react();
      }

      if (!d.pixelShaderPath.empty())
      {
        auto shader = m_shaders.shader(ShaderCreateInfo(d.pixelShaderPath, ShaderType::Pixel, d.layout));
        blobs.emplace_back(shader);
        desc.PS.BytecodeLength = blobs.back().size();
        desc.PS.pShaderBytecode = blobs.back().data();
        if (vp.ps.empty())
        {
          vp.ps = m_shaders.watch(d.pixelShaderPath, ShaderType::Pixel);
        }
        vp.ps.react();
      }

      ComPtr<ID3D12RootSignature> root;
      HIGANBANA_CHECK_HR(m_device->CreateRootSignature(m_nodeMask, desc.VS.pShaderBytecode, desc.VS.BytecodeLength, IID_PPV_ARGS(&root)));
      //desc.pRootSignature = root.Get();
    
      desc.pRootSignature = nullptr;

      ComPtr<ID3D12PipelineState> pipe;
      HIGANBANA_CHECK_HR(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipe)));

      D3D12_PRIMITIVE_TOPOLOGY primitive = convertPrimitiveTopology(d.primitiveTopology);

      if (vp.m_hasPipeline)
      {
        oldPipe = DX12OldPipeline{vp.pipeline, vp.root};
      }

      vp.pipeline = pipe;
      vp.root = root;
      vp.primitive = primitive;
      vp.m_hasPipeline = true;
      return oldPipe;
    }
  
    std::optional<DX12OldPipeline> DX12Device::updatePipeline(ResourceHandle pipeline)
    {
      std::optional<DX12OldPipeline> oldPipe;

      auto& pipe = m_allRes.pipelines[pipeline];

      if (pipe.m_hasPipeline && !pipe.cs.updated())
        return {};
      oldPipe = {pipe.pipeline, pipe.root};
      if (pipe.cs.updated())
      {
        GFX_LOG("Updating Compute pipeline %s", pipe.m_computeDesc.shaderSourcePath.c_str());
      }
      else
      {
        GFX_LOG("First time create Compute pipeline %s", pipe.m_computeDesc.shaderSourcePath.c_str());
      }

      ShaderCreateInfo sci = ShaderCreateInfo(pipe.m_computeDesc.shaderSourcePath, ShaderType::Compute, pipe.m_computeDesc.layout)
        .setComputeGroups(pipe.m_computeDesc.shaderGroups);

      auto thing = m_shaders.shader(sci);
      HIGANBANA_CHECK_HR(m_device->CreateRootSignature(m_nodeMask, thing.data(), thing.size(), IID_PPV_ARGS(&pipe.root)));

      D3D12_SHADER_BYTECODE byte;
      byte.pShaderBytecode = thing.data();
      byte.BytecodeLength = thing.size();

      if (pipe.cs.empty())
      {
        pipe.cs = m_shaders.watch(pipe.m_computeDesc.shaderSourcePath, ShaderType::Compute);
      }
      pipe.cs.react();

      D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc{};
      computeDesc.CS = byte;
      computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
      computeDesc.NodeMask = 0;
      //computeDesc.pRootSignature = ptr->root.Get();
      computeDesc.pRootSignature = nullptr;
      pipe.m_hasPipeline = true;

      HIGANBANA_CHECK_HR(m_device->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&pipe.pipeline)));
      return oldPipe;
    }

    void DX12Device::createHeap(ResourceHandle handle, HeapDescriptor heapDesc)
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

      ComPtr<ID3D12Heap> heap;
      m_device->CreateHeap(&dxdesc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf()));

      m_allRes.heaps[handle] = DX12Heap(heap);
    }

    void DX12Device::createBuffer(ResourceHandle handle, ResourceDescriptor& desc)
    {
      auto dxDesc = fillPlacedBufferInfo(desc);

      D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATE_COMMON;
      DX12ResourceState state{ 0 };
      state.commonStateOptimisation = true;
      state.flags.emplace_back(startState);
      ID3D12Resource* buffer;

      D3D12_HEAP_PROPERTIES prop{};
      prop.Type = D3D12_HEAP_TYPE_DEFAULT;

      m_device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED, &dxDesc, startState, nullptr, IID_PPV_ARGS(&buffer));

      auto wstr = s2ws(desc.desc.name);
      buffer->SetName(wstr.c_str());

      m_allRes.buf[handle] = DX12Buffer(buffer,desc);
    }

    void DX12Device::createBuffer(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc)
    {
      HIGAN_ASSERT(handle.type == ResourceType::Buffer, "handle should be correct");
      auto& native = m_allRes.heaps[allocation.heap.handle];
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
      DX12ResourceState state{ 0 };
      state.commonStateOptimisation = true;
      state.flags.emplace_back(startState);
      ID3D12Resource* buffer;
      m_device->CreatePlacedResource(native.native(), allocation.allocation.block.offset, &dxDesc, startState, nullptr, IID_PPV_ARGS(&buffer));

      auto wstr = s2ws(desc.desc.name);
      buffer->SetName(wstr.c_str());

      m_allRes.buf[handle] = DX12Buffer(buffer, desc);
    }

    void DX12Device::createBufferView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& bufferDesc, ShaderViewDescriptor& viewDesc)
    {
      auto& native = m_allRes.buf[buffer];

      auto desc = bufferDesc.desc;

      FormatType format = viewDesc.m_format;
      if (format == FormatType::Unknown)
        format = desc.format;

      auto descriptor = m_generics.allocate();

      auto sizeElements = viewDesc.m_elementCount;
      if (sizeElements == -1)
      {
        sizeElements = bufferDesc.desc.width;
      }

      if (viewDesc.m_format != FormatType::Unknown)
      {
        sizeElements = sizeElements * bufferDesc.desc.stride / formatSizeInfo(viewDesc.m_format).pixelSize;
      }

      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        D3D12_SHADER_RESOURCE_VIEW_DESC natDesc{};
        natDesc.Format = formatTodxFormat(format).view;
        natDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        natDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        natDesc.Buffer.FirstElement = viewDesc.m_firstElement;
        natDesc.Buffer.NumElements = sizeElements;
        natDesc.Buffer.StructureByteStride = (format == FormatType::Unknown) ? desc.stride : 0;
        natDesc.Buffer.Flags = (format == FormatType::Raw32) ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
        m_device->CreateShaderResourceView(native.native(), &natDesc, descriptor.cpu);
        m_allRes.bufSRV[handle] = DX12BufferView(descriptor);
      }
      else
      {
        D3D12_UNORDERED_ACCESS_VIEW_DESC natDesc{};
        natDesc.Format = formatTodxFormat(format).view;
        natDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        natDesc.Buffer.FirstElement = viewDesc.m_firstElement;
        natDesc.Buffer.NumElements = sizeElements;
        natDesc.Buffer.StructureByteStride = (format == FormatType::Unknown) ? desc.stride : 0;
        natDesc.Buffer.CounterOffsetInBytes = 0;
        natDesc.Buffer.Flags = (format == FormatType::Raw32) ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
        m_device->CreateUnorderedAccessView(native.native(), nullptr, &natDesc, descriptor.cpu);
        m_allRes.bufUAV[handle] = DX12BufferView(descriptor);
      }

    }

#define DX12CheckSupport1(s) checkSupport1(#s, s)
#define DX12CheckSupport2(s) checkSupport2(#s, s)
    void checkFormatOperationSupport(ID3D12Device* dev, FormatType format)
    {
      D3D12_FEATURE_DATA_FORMAT_SUPPORT data{};
      data.Format = formatTodxFormat(format).view;

      HRESULT hr = dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data));
      if (hr == S_OK)
      {
        std::string output = "";
        std::string output2 = "";
        auto checkSupport1 = [&](const char* s, D3D12_FORMAT_SUPPORT1 mask)
        {
          if (data.Support1 & mask)
          {
            output += s;
            output += "\n";
          }
        };
        auto checkSupport2 = [&](const char* s, D3D12_FORMAT_SUPPORT2 mask)
        {
          if (data.Support2 & mask)
          {
            output2 += s;
            output2 += "\n";
          }
        };
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_DISPLAY);
        /*
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_TEXTURE1D);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_TEXTURE2D);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_TEXTURE3D);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_TEXTURECUBE);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_SHADER_LOAD);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_COMPARISON);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_MONO_TEXT);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_MIP);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_BLENDABLE);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE);

        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_CAST_WITHIN_BIT_LAYOUT);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_SHADER_GATHER);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_BACK_BUFFER_CAST);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_SHADER_GATHER_COMPARISON);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_DECODER_OUTPUT);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_OUTPUT);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_INPUT);
        DX12CheckSupport1(D3D12_FORMAT_SUPPORT1_VIDEO_ENCODER);

        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_OUTPUT_MERGER_LOGIC_OP);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_TILED);
        DX12CheckSupport2(D3D12_FORMAT_SUPPORT2_MULTIPLANE_OVERLAY);  */
        //HIGAN_ILOG("DX12", "%s supports: \n%s%s", formatToString(format), output.c_str(), output2.c_str());
      }
    }

    void DX12Device::createTexture(ResourceHandle handle, ResourceDescriptor& desc)
    {
      HIGAN_ASSERT(handle.type == ResourceType::Texture, "handle should be correct");
      auto dxDesc = fillPlacedTextureInfo(desc);
      D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATE_COMMON;

      DX12ResourceState state{ 0 };
      if (desc.desc.allowCrossAdapter)
      {
        state.commonStateOptimisation = true; // usually false, to get those layout optimisations.
        dxDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
      }
      for (unsigned slice = 0; slice < desc.desc.arraySize; ++slice)
      {
        for (unsigned mip = 0; mip < desc.desc.miplevels; ++mip)
        {
          state.flags.emplace_back(startState);
        }
      }

      D3D12_CLEAR_VALUE clear;
      clear.Format = formatTodxFormat(desc.desc.format).view;
      D3D12_CLEAR_VALUE* clearPtr = nullptr;
      switch (desc.desc.usage)
      {
      case ResourceUsage::DepthStencil:
      case ResourceUsage::DepthStencilRW:
        clear.DepthStencil.Depth = 0.f;
        clear.DepthStencil.Stencil = 0;
        clearPtr = &clear;
        break;
      case ResourceUsage::RenderTarget:
      case ResourceUsage::RenderTargetRW:
        clear.Color[0] = 0.f;
        clear.Color[1] = 0.f;
        clear.Color[2] = 0.f;
        clear.Color[3] = 0.f;
        clearPtr = &clear;
        break;
      default:
        break;
      }

      ID3D12Resource* texture;
      D3D12_HEAP_PROPERTIES prop{};
      prop.Type = D3D12_HEAP_TYPE_DEFAULT;


      for (int i = 0; i < static_cast<int>(FormatType::Count); ++i)
      {
        checkFormatOperationSupport(m_device.Get(), static_cast<FormatType>(i));
      }

      dxDesc.Format = formatTodxFormat(desc.desc.format).storage;

      D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
      if (desc.desc.allowCrossAdapter)
      {
        dxDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // forced
        flags = D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED;
      }

      m_device->CreateCommittedResource(&prop, flags, &dxDesc, startState, nullptr, IID_PPV_ARGS(&texture));

      auto wstr = s2ws(desc.desc.name);
      texture->SetName(wstr.c_str());

      m_allRes.tex[handle] = DX12Texture(texture, desc, desc.desc.miplevels);
    }

    void DX12Device::createTexture(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc)
    {
      HIGAN_ASSERT(handle.type == ResourceType::Texture, "handle should be correct");
      auto& native = m_allRes.heaps[allocation.heap.handle];
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

      DX12ResourceState state{ 0 };
      state.commonStateOptimisation = false; // usually false, to get those layout optimisations.
      for (unsigned slice = 0; slice < desc.desc.arraySize; ++slice)
      {
        for (unsigned mip = 0; mip < desc.desc.miplevels; ++mip)
        {
          state.flags.emplace_back(startState);
        }
      }

      D3D12_CLEAR_VALUE clear;
      clear.Format = formatTodxFormat(desc.desc.format).view;
      D3D12_CLEAR_VALUE* clearPtr = nullptr;
      switch (desc.desc.usage)
      {
      case ResourceUsage::DepthStencil:
      case ResourceUsage::DepthStencilRW:
        clear.DepthStencil.Depth = 0.f;
        clear.DepthStencil.Stencil = 0;
        clearPtr = &clear;
        break;
      case ResourceUsage::RenderTarget:
      case ResourceUsage::RenderTargetRW:
        clear.Color[0] = 0.f;
        clear.Color[1] = 0.f;
        clear.Color[2] = 0.f;
        clear.Color[3] = 0.f;
        clearPtr = &clear;
        break;
      default:
        break;
      }

      ID3D12Resource* texture;
      m_device->CreatePlacedResource(native.native(), allocation.allocation.block.offset, &dxDesc, startState, clearPtr, IID_PPV_ARGS(&texture));

      auto wstr = s2ws(desc.desc.name);
      texture->SetName(wstr.c_str());

      m_allRes.tex[handle] = DX12Texture(texture, desc, desc.desc.miplevels);
    }

    void DX12Device::createTextureView(ViewResourceHandle handle, ResourceHandle texture, ResourceDescriptor& texDesc, ShaderViewDescriptor& viewDesc)
    {
      //auto native = std::static_pointer_cast<DX12Texture>(texture);
      auto& native = m_allRes.tex[texture];

      DX12CPUDescriptor descriptor{};

      /*
      unsigned mips = (viewDesc.m_viewType == ResourceShaderType::ReadOnly) ? texDesc.desc.miplevels - viewDesc.m_mostDetailedMip : 1;
      if (viewDesc.m_mipLevels != -1)
      {
        mips = viewDesc.m_mipLevels;
      }
      unsigned arraySize = (viewDesc.m_arraySize != -1) ? viewDesc.m_arraySize : texDesc.desc.arraySize - viewDesc.m_arraySlice;
      SubresourceRange range{};
      range.mipOffset = static_cast<int16_t>(viewDesc.m_mostDetailedMip);
      range.mipLevels = static_cast<int16_t>(mips);
      range.sliceOffset = static_cast<int16_t>(viewDesc.m_arraySlice);
      range.arraySize = static_cast<int16_t>(arraySize);
      */

      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        descriptor = m_generics.allocate();
        auto natDesc = dx12::getSRV(texDesc, viewDesc);
        m_device->CreateShaderResourceView(native.native(), &natDesc, descriptor.cpu);
        m_allRes.texSRV[handle] = DX12TextureView(descriptor, natDesc.Format);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::ReadWrite)
      {
        descriptor = m_generics.allocate();
        auto natDesc = dx12::getUAV(texDesc, viewDesc);
        m_device->CreateUnorderedAccessView(native.native(), nullptr, &natDesc, descriptor.cpu);
        m_allRes.texUAV[handle] = DX12TextureView(descriptor, natDesc.Format);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::RenderTarget)
      {
        descriptor = m_rtvs.allocate();
        auto natDesc = dx12::getRTV(texDesc, viewDesc);
        m_device->CreateRenderTargetView(native.native(), &natDesc, descriptor.cpu);
        m_allRes.texRTV[handle] = DX12TextureView(descriptor, natDesc.Format);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::DepthStencil)
      {
        descriptor = m_dsvs.allocate();
        auto natDesc = dx12::getDSV(texDesc, viewDesc);
        m_device->CreateDepthStencilView(native.native(), &natDesc, descriptor.cpu);
        m_allRes.texDSV[handle] = DX12TextureView(descriptor, natDesc.Format);
      }
      else
      {
        HIGAN_ASSERT(false, "WTF!");
      }
    }

    void DX12Device::dynamic(ViewResourceHandle handle, MemView<uint8_t> view, FormatType type)
    {
      HIGAN_ASSERT(handle.type == ViewResourceType::DynamicBufferSRV, "handle should be correct");
      auto descriptor = m_generics.allocate();
      auto upload = m_dynamicUpload->allocate(view.size());
      HIGAN_ASSERT(upload, "Halp");
      memcpy(upload.data(), view.data(), view.size());

      auto format = formatTodxFormat(type).view;
      auto stride = formatSizeInfo(type).pixelSize;

      D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
      desc.Format = format;
      desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      desc.Buffer.NumElements = static_cast<unsigned>(view.size() / stride);
      desc.Buffer.FirstElement = upload.block.offset / stride;
      desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      if (type == FormatType::Raw32)
        desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
      desc.Buffer.StructureByteStride = format == DXGI_FORMAT_UNKNOWN ? stride : 0;
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

      HIGAN_ASSERT(upload.block.offset % stride == 0, "oh no");
      m_device->CreateShaderResourceView(m_dynamicUpload->native(), &desc, descriptor.cpu);

      m_allRes.dynSRV[handle] = DX12DynamicBufferView(upload, descriptor, format, stride);
    }

    void DX12Device::dynamic(ViewResourceHandle handle, MemView<uint8_t> view, unsigned stride)
    {
      HIGAN_ASSERT(handle.type == ViewResourceType::DynamicBufferSRV, "handle should be correct");
      auto descriptor = m_generics.allocate();
      auto upload = m_dynamicUpload->allocate(view.size());
      HIGAN_ASSERT(upload, "Halp");
      memcpy(upload.data(), view.data(), view.size());
      D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      desc.Buffer.NumElements = static_cast<unsigned>(view.size() / stride);
      desc.Buffer.FirstElement = upload.block.offset / stride;
      desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      desc.Buffer.StructureByteStride = stride;
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

      HIGAN_ASSERT(upload.block.offset % stride == 0, "oh no");

      m_device->CreateShaderResourceView(m_dynamicUpload->native(), &desc, descriptor.cpu);

      m_allRes.dynSRV[handle] = DX12DynamicBufferView(upload, descriptor, DXGI_FORMAT_UNKNOWN, stride);
    }

    void DX12Device::dynamicImage(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned rowPitch)
    {
      HIGAN_ASSERT(handle.type == ViewResourceType::DynamicBufferSRV, "handle should be correct");
      auto rows = bytes.size() / rowPitch;

      constexpr auto APIRowPitchAlignmentRequirement = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

      const auto requiredRowPitch = roundUpMultiplePowerOf2(rowPitch, APIRowPitchAlignmentRequirement);
      HIGAN_LOG("rowPitch %zu\n", requiredRowPitch);
      const auto requiredTotalSize = rows * requiredRowPitch;

      auto upload = m_dynamicUpload->allocate(requiredTotalSize);
      HIGAN_ASSERT(upload, "Halp");
      for (size_t row = 0; row < rows; ++row)
      {
        auto srcPosition = rowPitch * row;
        auto dstPosition = requiredRowPitch * row;
        memcpy(upload.data() + dstPosition, bytes.data() + srcPosition, rowPitch);
      }
      m_allRes.dynSRV[handle] = DX12DynamicBufferView(upload, requiredRowPitch);
    }

    void DX12Device::readbackBuffer(ResourceHandle readback, size_t bytes)
    {
      // get description of source and allocate readback memory based on the bytes required
    }

    DX12QueryHeap DX12Device::createGraphicsQueryHeap(unsigned counters)
    {
      return DX12QueryHeap(m_device.Get(), m_graphicsQueue.Get(), counters);
    }

    DX12QueryHeap DX12Device::createComputeQueryHeap(unsigned counters)
    {
      return DX12QueryHeap(m_device.Get(), m_computeQueue.Get(), counters);
    }

    DX12ReadbackHeap DX12Device::createReadback(unsigned pages, unsigned pageSize)
    {
      return DX12ReadbackHeap(m_device.Get(), pages, pageSize);
    }

    DX12CommandBuffer DX12Device::createList(D3D12_COMMAND_LIST_TYPE type)
    {
      ComPtr<D3D12GraphicsCommandList> commandList;
      ComPtr<ID3D12CommandAllocator> commandListAllocator;
      HIGANBANA_CHECK_HR(m_device->CreateCommandAllocator(type, IID_PPV_ARGS(commandListAllocator.ReleaseAndGetAddressOf())));
      HIGANBANA_CHECK_HR(m_device->CreateCommandList(1, type, commandListAllocator.Get(), NULL, IID_PPV_ARGS(commandList.GetAddressOf())));

      return DX12CommandBuffer(commandList, commandListAllocator, type == D3D12_COMMAND_LIST_TYPE_COPY);
    }

    DX12Fence DX12Device::createNativeFence()
    {
      ComPtr<ID3D12Fence> fence;
      HIGANBANA_CHECK_HR(m_device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
      return DX12Fence(fence);
    }

    DX12Semaphore DX12Device::createNativeSemaphore()
    {
      ComPtr<ID3D12Fence> fence;
      HIGANBANA_CHECK_HR(m_device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
      return DX12Semaphore(fence);
    }

    // commandlist things and gpu-cpu/gpu-gpu synchronization primitives
    std::shared_ptr<CommandBufferImpl> DX12Device::createDMAList()
    {
      //auto seqNumber = m_seqTracker->next();
      //std::shared_ptr<SequenceTracker> tracker = m_seqTracker;
      auto list = std::shared_ptr<DX12CommandList>(new DX12CommandList(
        m_copyListPool.allocate(),
        m_constantsUpload,
        m_dynamicUpload,
        m_readbackPool.allocate(),
        m_queryHeapPool.allocate(),
        m_dynamicGpuDescriptors,
        m_nullBufferUAV,
        m_nullBufferSRV),
        [](DX12CommandList* ptr)
      {
        /*
        if (auto seqTracker = tracker)
        {
          seqTracker->complete(seqNumber);
        }
        */
        delete ptr;
      });

      return list;
    }
    std::shared_ptr<CommandBufferImpl> DX12Device::createComputeList()
    {
      //auto seqNumber = m_seqTracker->next();
      //std::shared_ptr<SequenceTracker> tracker = m_seqTracker;
      auto list = std::shared_ptr<DX12CommandList>(new DX12CommandList(
        m_computeListPool.allocate(),
        m_constantsUpload,
        m_dynamicUpload,
        m_readbackPool.allocate(),
        m_computeQueryHeapPool.allocate(),
        m_dynamicGpuDescriptors,
        m_nullBufferUAV,
        m_nullBufferSRV),
        [](DX12CommandList* ptr)
      {
        /*
        if (auto seqTracker = tracker)
        {
          seqTracker->complete(seqNumber);
        }
        */
        delete ptr;
      });
      return list;
    }
    std::shared_ptr<CommandBufferImpl> DX12Device::createGraphicsList()
    {
      //auto seqNumber = m_seqTracker->next();
      //std::shared_ptr<SequenceTracker> tracker = m_seqTracker;
      auto list = std::shared_ptr<DX12CommandList>(new DX12CommandList(
        m_graphicsListPool.allocate(),
        m_constantsUpload,
        m_dynamicUpload,
        m_readbackPool.allocate(),
        m_queryHeapPool.allocate(),
        m_dynamicGpuDescriptors,
        m_nullBufferUAV,
        m_nullBufferSRV),
        [](DX12CommandList* ptr)
      {
        /*
        if (auto seqTracker = tracker)
        {
          seqTracker->complete(seqNumber);
        }*/
        delete ptr;
      });
      return list;
    }

    std::shared_ptr<SemaphoreImpl> DX12Device::createSemaphore()
    {
      return m_semaPool.allocate();
    }
    std::shared_ptr<FenceImpl> DX12Device::createFence()
    {
      return m_fencePool.allocate();
    }

    void DX12Device::submit(
      ComPtr<ID3D12CommandQueue> queue,
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<std::shared_ptr<FenceImpl>>         fence)
    {
      if (!wait.empty())
      {
        for (auto&& sema : wait)
        {
          auto native = std::static_pointer_cast<DX12Semaphore>(sema);
          queue->Wait(native->fence.Get(), *native->value);
        }
      }
      std::vector<ID3D12CommandList*> natList;
      if (!lists.empty())
      {
        for (auto&& buffer : lists)
        {
          auto native = std::static_pointer_cast<DX12CommandList>(buffer);
          if (native->closed())
            natList.emplace_back(native->list());
          else
            HIGAN_ASSERT(false, "Remove when you feel like it.");
        }
      }
      if (!natList.empty())
        queue->ExecuteCommandLists(static_cast<UINT>(natList.size()), natList.data());

      if (!signal.empty())
      {
        for (auto&& sema : signal)
        {
          auto native = std::static_pointer_cast<DX12Semaphore>(sema);
          queue->Signal(native->fence.Get(), native->start());
        }
      }
      if (!fence.empty())
      {
        for (auto&& fe : fence)
        {
          auto native = std::static_pointer_cast<DX12Fence>(fe);
          auto value = native->start();
          queue->Signal(native->fence.Get(), value);
        }
      }
    }

    void DX12Device::submitDMA(
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<std::shared_ptr<FenceImpl>>         fence)
    {
      submit(m_dmaQueue, std::forward<decltype(lists)>(lists), std::forward<decltype(wait)>(wait), std::forward<decltype(signal)>(signal), std::forward<decltype(fence)>(fence));
    }

    void DX12Device::submitCompute(
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<std::shared_ptr<FenceImpl>>         fence)
    {
      submit(m_computeQueue, std::forward<decltype(lists)>(lists), std::forward<decltype(wait)>(wait), std::forward<decltype(signal)>(signal), std::forward<decltype(fence)>(fence));
    }

    void DX12Device::submitGraphics(
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<std::shared_ptr<FenceImpl>>         fence)
    {
      submit(m_graphicsQueue, std::forward<decltype(lists)>(lists), std::forward<decltype(wait)>(wait), std::forward<decltype(signal)>(signal), std::forward<decltype(fence)>(fence));
    }

    void DX12Device::waitFence(std::shared_ptr<FenceImpl> fence)
    {
      auto native = std::static_pointer_cast<DX12Fence>(fence);
      native->waitTillReady();
    }

    bool DX12Device::checkFence(std::shared_ptr<FenceImpl> fence)
    {
      auto native = std::static_pointer_cast<DX12Fence>(fence);
      return native->hasCompleted();
    }

    void DX12Device::present(std::shared_ptr<prototypes::SwapchainImpl> swapchain, std::shared_ptr<SemaphoreImpl> renderingFinished)
    {
      if (renderingFinished)
      {
        auto native = std::static_pointer_cast<DX12Semaphore>(renderingFinished);
        m_graphicsQueue->Wait(native->fence.Get(), *native->value);
      }
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);
      unsigned syncInterval = 0;
      uint flag = 0;
      auto mode = native->getDesc().descriptor.desc.mode;
      if (mode == PresentMode::Fifo) {
        syncInterval = 1;
      } else {
        flag = DXGI_PRESENT_ALLOW_TEARING;
      }
      HIGANBANA_CHECK_HR(native->native()->Present(syncInterval, flag));
    }

    std::shared_ptr<SemaphoreImpl> DX12Device::createSharedSemaphore()
    {
      ComPtr<ID3D12Fence> fence;
      HIGANBANA_CHECK_HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER | D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
      return std::make_shared<DX12Semaphore>(fence);
    }

    std::shared_ptr<backend::SharedHandle> DX12Device::openSharedHandle(std::shared_ptr<backend::SemaphoreImpl> sema)
    {
      auto native = std::static_pointer_cast<DX12Semaphore>(sema);

      HANDLE h;

      HIGANBANA_CHECK_HR(m_device->CreateSharedHandle(native->fence.Get(), nullptr, GENERIC_ALL, L"sharedHandle_semaphore_Higanbana", &h));

      return std::shared_ptr<SharedHandle>(new SharedHandle{ GraphicsApi::DX12, h }, [](SharedHandle* ptr)
      {
        CloseHandle(ptr->handle);
        delete ptr;
      });
    }

    std::shared_ptr<SharedHandle> DX12Device::openSharedHandle(HeapAllocation heapAllocation)
    {
      auto& native = m_allRes.heaps[heapAllocation.heap.handle];

      HANDLE h;

      HIGANBANA_CHECK_HR(m_device->CreateSharedHandle(native.native(), nullptr, GENERIC_ALL, L"sharedHandle_heap_Higanbana", &h));

      return std::shared_ptr<SharedHandle>(new SharedHandle{GraphicsApi::DX12, h, heapAllocation.heap.desc->desc.sizeInBytes }, [](SharedHandle* ptr)
      {
        CloseHandle(ptr->handle);
        delete ptr;
      });
    }

    std::shared_ptr<SharedHandle> DX12Device::openSharedHandle(ResourceHandle resource)
    {
      HANDLE h;
      size_t size;

      if (resource.type == ResourceType::Texture)
      {
        auto& native = m_allRes.tex[resource];
        HIGANBANA_CHECK_HR(m_device->CreateSharedHandle(native.native(), nullptr, GENERIC_ALL, L"sharedHandle_texture_Higanbana", &h));
      }

      return std::shared_ptr<SharedHandle>(new SharedHandle{GraphicsApi::DX12, h, 0 }, [](SharedHandle* ptr)
      {
        CloseHandle(ptr->handle);
        delete ptr;
      });
    }

    std::shared_ptr<backend::SharedHandle> DX12Device::openForInteropt(ResourceHandle resource)
    {
      HANDLE h;

      if (resource.type == ResourceType::Texture)
      {
        auto& native = m_allRes.tex[resource];
        HIGANBANA_CHECK_HR(m_device->CreateSharedHandle(native.native(), nullptr, MAXIMUM_ALLOWED, L"sharedHandle_texture_Higanbana", &h));
      }

      return std::shared_ptr<SharedHandle>(new SharedHandle{GraphicsApi::DX12, h, 0 }, [](SharedHandle* ptr)
      {
        CloseHandle(ptr->handle);
        delete ptr;
      });
    }

    std::shared_ptr<backend::SemaphoreImpl> DX12Device::createSemaphoreFromHandle(std::shared_ptr<backend::SharedHandle> handle)
    {
      ComPtr<ID3D12Fence> fence;
      m_device->OpenSharedHandle(handle->handle, IID_PPV_ARGS(fence.GetAddressOf()));
      return std::make_shared<DX12Semaphore>(fence);
    }

    void DX12Device::createHeapFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared)
    {
      ID3D12Heap* heap;
      m_device->OpenSharedHandle(shared->handle, IID_PPV_ARGS(&heap));
      m_allRes.heaps[handle] = DX12Heap(heap);
    }
      
    void DX12Device::createBufferFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared, HeapAllocation heapAllocation, ResourceDescriptor& desc)
    {
      ID3D12Heap* heap;
      m_device->OpenSharedHandle(shared->handle, IID_PPV_ARGS(&heap));

      auto dxDesc = fillPlacedBufferInfo(desc);

      D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATE_COMMON;
      DX12ResourceState state{ 0 };
      state.commonStateOptimisation = true;
      state.flags.emplace_back(startState);
      ID3D12Resource* buffer;
      m_device->CreatePlacedResource(heap, heapAllocation.allocation.block.offset, &dxDesc, startState, nullptr, IID_PPV_ARGS(&buffer));

      auto wstr = s2ws(desc.desc.name);
      buffer->SetName(wstr.c_str());
      
      m_allRes.buf[handle] = DX12Buffer(buffer, desc);
    }
    
    void DX12Device::createTextureFromHandle(ResourceHandle handle, std::shared_ptr<backend::SharedHandle> shared, ResourceDescriptor& desc)
    {
      ID3D12Resource* texture;
      m_device->OpenSharedHandle(shared->handle, IID_PPV_ARGS(&texture));

      D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATE_COMMON;

      DX12ResourceState state{ 0 };
      state.commonStateOptimisation = false; // usually false, to get those layout optimisations.
      for (unsigned slice = 0; slice < desc.desc.arraySize; ++slice)
      {
        for (unsigned mip = 0; mip < desc.desc.miplevels; ++mip)
        {
          state.flags.emplace_back(startState);
        }
      }

      auto wstr = s2ws(desc.desc.name);
      texture->SetName(wstr.c_str());

      m_allRes.tex[handle] = DX12Texture(texture, desc, desc.desc.miplevels);
    }
  }
}
#endif