#if defined(FAZE_PLATFORM_WINDOWS)
#include "dx12resources.hpp"
#include "util/formats.hpp"
#include "faze/src/new_gfx/common/graphicssurface.hpp"
#include "view_descriptor.hpp"
#include "core/src/system/bitpacking.hpp"
#include "faze/src/new_gfx/common/commandpackets.hpp"

#include "core/src/global_debug.hpp"
#include "faze/src/new_gfx/definitions.hpp"

#include "faze/src/new_gfx/dx12/util/pipeline_helpers.hpp"

#include "faze/src/new_gfx/dx12/util/dxDependencySolver.hpp"

#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
#include <DXGIDebug.h>
#endif

namespace faze
{
  namespace backend
  {
    DX12Device::DX12Device(GpuInfo info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory4> factory, FileSystem& fs)
      : m_info(info)
      , m_device(device)
      , m_factory(factory)
      , m_fs(fs)
      , m_shaders(fs, "shaders", "shaders")
      , m_nodeMask(0) // sli/crossfire index
      , m_generics(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024) // lol 1024, right.
      , m_samplers(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16)
      , m_rtvs(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64)
      , m_dsvs(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 16)
      , m_constantsUpload(std::make_shared<DX12UploadHeap>(device.Get(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 64, 512)) // we have room for 64*512 drawcalls worth of constants.
      , m_dynamicUpload(std::make_shared<DX12UploadHeap>(device.Get(), 256 * 256, 1024 * 4)) // we have room 64megs of dynamic buffers
      , m_dynamicReadback(std::make_shared<DX12ReadbackHeap>(device.Get(), 256 * 256, 1024 * 4)) // we have room 64megs of dynamic buffers
      , m_dynamicGpuDescriptors(std::make_shared<DX12DynamicDescriptorHeap>(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, 480))
      , m_trash(std::make_shared<Garbage>())
      , m_seqTracker(std::make_shared<SequenceTracker>())
    {
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
      DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_debug));
      //m_debug->EnableLeakTrackingForThread();
#endif
      D3D12_COMMAND_QUEUE_DESC desc{};
      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
      desc.NodeMask = m_nodeMask;
      FAZE_CHECK_HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_graphicsQueue.ReleaseAndGetAddressOf())));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      FAZE_CHECK_HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_dmaQueue.ReleaseAndGetAddressOf())));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      FAZE_CHECK_HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_computeQueue.ReleaseAndGetAddressOf())));

      m_deviceFence = createNativeFence();

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
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
      m_debug->ReportLiveObjects(DXGI_DEBUG_DX, DXGI_DEBUG_RLO_ALL);
#endif
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

    std::shared_ptr<prototypes::SwapchainImpl> DX12Device::createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor)
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
      swapChainDesc.Format = formatTodxFormat(descriptor.desc.format).storage;
      swapChainDesc.Stereo = FALSE;
      swapChainDesc.SampleDesc.Count = 1;
      swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc.BufferCount = static_cast<UINT>(descriptor.desc.bufferCount); // conviently array can be used to describe bufferCount
      swapChainDesc.Scaling = DXGI_SCALING_NONE; // scaling.. hmm ignore for now
      swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // just use flip_sequential, DXGI_SWAP_EFFECT_FLIP_DISCARD
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
      FAZE_CHECK_HR(m_factory->CreateSwapChainForHwnd(m_graphicsQueue.Get(), natSurface->native(), &swapChainDesc, nullptr, nullptr, &base_chain));
      FAZE_CHECK_HR(base_chain.As(&swapChain));
      FAZE_CHECK_HR(swapChain->SetMaximumFrameLatency(descriptor.desc.frameLatency));

      std::shared_ptr<DX12Swapchain> sc;
      HANDLE thingy = swapChain->GetFrameLatencyWaitableObject();
      sc = std::make_shared<DX12Swapchain>(swapChain, *natSurface, thingy);

      sc->setBufferMetadata(width, height, descriptor);
      m_factory->MakeWindowAssociation(natSurface->native(), DXGI_MWA_NO_WINDOW_CHANGES); // if using alt+enter, we would still need to call ResizeBuffers

#if !defined(FAZE_NSIGHT_COMPATIBILITY)
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
      F_ASSERT(lol, "window rect failed ....?");
      auto width = rect.right - rect.left;
      auto height = rect.bottom - rect.top;
      //F_SLOG("DX12", "adjusting swapchain to %ux%u\n", width, height);

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
      FAZE_CHECK_HR(m_factory->CreateSwapChainForHwnd(m_graphicsQueue.Get(), natSwapchain->surface().native(), &swapChainDesc, nullptr, nullptr, &base_chain));
      FAZE_CHECK_HR(base_chain.As(&swapChain));
      FAZE_CHECK_HR(swapChain->SetMaximumFrameLatency(d.desc.frameLatency));

      natSwapchain->setSwapchain(swapChain);

      HANDLE thingy = natSwapchain->native()->GetFrameLatencyWaitableObject();
      natSwapchain->setFrameLatencyObj(thingy);

      m_factory->MakeWindowAssociation(natSwapchain->surface().native(), DXGI_MWA_NO_WINDOW_CHANGES); // if using alt+enter, we would still need to call ResizeBuffers

      natSwapchain->setBufferMetadata(width, height, d);

#if !defined(FAZE_NSIGHT_COMPATIBILITY)
      ensureSwapchainColorspace(natSwapchain, d);
#endif
    }

    void DX12Device::ensureSwapchainColorspace(std::shared_ptr<DX12Swapchain> sc, SwapchainDescriptor& descriptor)
    {
      if (!m_factory->IsCurrent())
      {
        FAZE_CHECK_HR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_factory)));
      }

      // Get information about the display we are presenting to.
      ComPtr<IDXGIOutput> output;
      ComPtr<IDXGIOutput6> output6;

      if (SUCCEEDED(sc->native()->GetContainingOutput(&output)))
      {
        if (SUCCEEDED(output.As(&output6)))
        {
          DXGI_OUTPUT_DESC1 outputDesc;
          FAZE_CHECK_HR(output6->GetDesc1(&outputDesc));

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
          FAZE_CHECK_HR(sc->native()->SetColorSpace1(colorSpace));
          sc->setNativeColorspace(colorSpace);
        }
      }
    }

    void DX12Device::destroySwapchain(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      //auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);
      //native->native()->Release();
    }

    vector<std::shared_ptr<prototypes::TextureImpl>> DX12Device::getSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);

      vector<std::shared_ptr<prototypes::TextureImpl>> textures;
      textures.resize(native->getDesc().descriptor.desc.bufferCount);

      DX12ResourceState state;
      state.flags.emplace_back(D3D12_RESOURCE_STATE_COMMON);

      std::weak_ptr<Garbage> weak = m_trash;

      for (int i = 0; i < native->getDesc().descriptor.desc.bufferCount; ++i)
      {
        ID3D12Resource* renderTarget;
        FAZE_CHECK_HR(native->native()->GetBuffer(i, IID_PPV_ARGS(&renderTarget)));

        textures[i] = std::shared_ptr<DX12Texture>(new DX12Texture(renderTarget, std::make_shared<DX12ResourceState>(state)),
          [weak](DX12Texture* ptr)
        {
          if (auto trash = weak.lock())
          {
            trash->resources.emplace_back(ptr->native());
          }
          else
          {
            ptr->native()->Release();
          }
          delete ptr;
        });
      }
      return textures;
    }

    int DX12Device::acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);
      native->waitForFrameLatency();
      int index = native->native()->GetCurrentBackBufferIndex();
      native->setBackbufferIndex(index);
      return index;
    }

    void DX12Device::collectTrash()
    {
      auto latestSequenceStarted = m_seqTracker->lastSequence();

      m_collectableTrash.emplace_back(std::make_pair(latestSequenceStarted, *m_trash));
      m_trash->dynamicBuffers.clear();
      m_trash->genericDescriptors.clear();
      m_trash->rtvsDescriptors.clear();
      m_trash->dsvsDescriptors.clear();
      m_trash->resources.clear();

      while (!m_collectableTrash.empty())
      {
        auto&& it = m_collectableTrash.front();
        if (!m_seqTracker->hasCompletedTill(it.first))
        {
          break;
        }
        for (auto&& dynBuf : it.second.dynamicBuffers)
        {
          m_dynamicUpload->release(dynBuf);
        }
        for (auto&& descriptor : it.second.genericDescriptors)
        {
          m_generics.release(descriptor);
        }
        for (auto&& descriptor : it.second.rtvsDescriptors)
        {
          m_rtvs.release(descriptor);
        }
        for (auto&& descriptor : it.second.dsvsDescriptors)
        {
          m_dsvs.release(descriptor);
        }
        for (auto&& resource : it.second.resources)
        {
          resource->Release();
        }
        m_collectableTrash.pop_front();
      }
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

    std::shared_ptr<prototypes::RenderpassImpl> DX12Device::createRenderpass()
    {
      return std::make_shared<DX12Renderpass>();
    }

    std::shared_ptr<prototypes::PipelineImpl> DX12Device::createPipeline()
    {
      return std::make_shared<DX12Pipeline>();
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC getDesc(GraphicsPipeline& pipeline, gfxpacket::Subpass& subpass)
    {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

      GraphicsPipelineDescriptor::Desc d = pipeline.descriptor;
      {
        // BlendState
        desc.BlendState.IndependentBlendEnable = d.blendDesc.desc.alphaToCoverageEnable;
        desc.BlendState.AlphaToCoverageEnable = d.blendDesc.desc.independentBlendEnable;
        for (int i = 0; i < 8; ++i)
        {
          auto& rtb = d.blendDesc.desc.renderTarget[i].desc;
          desc.BlendState.RenderTarget[i].BlendEnable = rtb.blendEnable;
          desc.BlendState.RenderTarget[i].LogicOpEnable = rtb.logicOpEnable;
          desc.BlendState.RenderTarget[i].SrcBlend = convertBlend(rtb.srcBlend);
          desc.BlendState.RenderTarget[i].DestBlend = convertBlend(rtb.destBlend);
          desc.BlendState.RenderTarget[i].BlendOp = convertBlendOp(rtb.blendOp);
          desc.BlendState.RenderTarget[i].SrcBlendAlpha = convertBlend(rtb.srcBlendAlpha);
          desc.BlendState.RenderTarget[i].DestBlendAlpha = convertBlend(rtb.destBlendAlpha);
          desc.BlendState.RenderTarget[i].BlendOpAlpha = convertBlendOp(rtb.blendOpAlpha);
          desc.BlendState.RenderTarget[i].LogicOp = convertLogicOp(rtb.logicOp);
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
      desc.NumRenderTargets = static_cast<unsigned>(subpass.rtvs.size());
      for (size_t i = 0; i < subpass.rtvs.size(); ++i)
      {
        desc.RTVFormats[i] = formatTodxFormat(subpass.rtvs[i].format()).view;
      }
      desc.DSVFormat = (subpass.dsvs.size() == 0) ? DXGI_FORMAT_UNKNOWN : formatTodxFormat(subpass.dsvs[0].format()).view;
      for (size_t i = subpass.rtvs.size(); i < 8; ++i)
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

    void DX12Device::updatePipeline(GraphicsPipeline& pipeline, gfxpacket::Subpass& subpass)
    {
      auto hash = subpass.hash;
      bool missing = true;

      GraphicsPipeline::FullPipeline* ptr = nullptr;

      for (auto&& it : *pipeline.m_pipelines)
      {
        if (it.hash == hash)
        {
          missing = false;
          ptr = &it;
          break;
        }
      }

      if (missing)
      {
        pipeline.m_pipelines->emplace_back(GraphicsPipeline::FullPipeline{});
        ptr = &pipeline.m_pipelines->back();
        ptr->hash = hash;
      }

      if (ptr->needsUpdating())
      {
        F_LOG("updating pipeline for hash %zu\n", hash);
        auto desc = getDesc(pipeline, subpass);

        GraphicsPipelineDescriptor::Desc d = pipeline.descriptor;
        vector<MemoryBlob> blobs;
        if (!d.vertexShaderPath.empty())
        {
          auto shader = m_shaders.shader(d.vertexShaderPath, DX12ShaderStorage::ShaderType::Vertex);
          blobs.emplace_back(shader);
          desc.VS.BytecodeLength = blobs.back().size();
          desc.VS.pShaderBytecode = blobs.back().data();

          if (ptr->vs.empty())
          {
            ptr->vs = m_shaders.watch(d.vertexShaderPath, DX12ShaderStorage::ShaderType::Vertex);
          }
          ptr->vs.react();
        }

        if (!d.hullShaderPath.empty())
        {
          auto shader = m_shaders.shader(d.hullShaderPath, DX12ShaderStorage::ShaderType::TessControl);
          blobs.emplace_back(shader);
          desc.HS.BytecodeLength = blobs.back().size();
          desc.HS.pShaderBytecode = blobs.back().data();

          if (ptr->hs.empty())
          {
            ptr->hs = m_shaders.watch(d.hullShaderPath, DX12ShaderStorage::ShaderType::TessControl);
          }
          ptr->hs.react();
        }

        if (!d.domainShaderPath.empty())
        {
          auto shader = m_shaders.shader(d.domainShaderPath, DX12ShaderStorage::ShaderType::TessEvaluation);
          blobs.emplace_back(shader);
          desc.DS.BytecodeLength = blobs.back().size();
          desc.DS.pShaderBytecode = blobs.back().data();
          if (ptr->ds.empty())
          {
            ptr->ds = m_shaders.watch(d.domainShaderPath, DX12ShaderStorage::ShaderType::TessEvaluation);
          }
          ptr->ds.react();
        }

        if (!d.geometryShaderPath.empty())
        {
          auto shader = m_shaders.shader(d.geometryShaderPath, DX12ShaderStorage::ShaderType::Geometry);
          blobs.emplace_back(shader);
          desc.GS.BytecodeLength = blobs.back().size();
          desc.GS.pShaderBytecode = blobs.back().data();
          if (ptr->gs.empty())
          {
            ptr->gs = m_shaders.watch(d.geometryShaderPath, DX12ShaderStorage::ShaderType::Geometry);
          }
          ptr->gs.react();
        }

        if (!d.pixelShaderPath.empty())
        {
          auto shader = m_shaders.shader(d.pixelShaderPath, DX12ShaderStorage::ShaderType::Pixel);
          blobs.emplace_back(shader);
          desc.PS.BytecodeLength = blobs.back().size();
          desc.PS.pShaderBytecode = blobs.back().data();
          if (ptr->ps.empty())
          {
            ptr->ps = m_shaders.watch(d.pixelShaderPath, DX12ShaderStorage::ShaderType::Pixel);
          }
          ptr->ps.react();
        }

        ComPtr<ID3D12RootSignature> root;
        FAZE_CHECK_HR(m_device->CreateRootSignature(m_nodeMask, desc.VS.pShaderBytecode, desc.VS.BytecodeLength, IID_PPV_ARGS(&root)));
        desc.pRootSignature = root.Get();

        ComPtr<ID3D12PipelineState> pipe;
        FAZE_CHECK_HR(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipe)));

        D3D12_PRIMITIVE_TOPOLOGY primitive = convertPrimitiveTopology(d.primitiveTopology);

        if (ptr->pipeline)
        {
          auto* natptr = static_cast<DX12Pipeline*>(ptr->pipeline.get());
          m_trash->pipelines.emplace_back(natptr->pipeline);
          m_trash->roots.emplace_back(natptr->root);
        }

        ptr->pipeline = std::make_shared<DX12Pipeline>(DX12Pipeline(pipe, root, primitive));
        //pipeline.m_pipelines->emplace_back(std::make_pair(hash, std::make_shared<DX12Pipeline>(DX12Pipeline(pipe, root, primitive))));
      }
    }

    void DX12Device::updatePipeline(ComputePipeline& pipeline)
    {
      auto* ptr = static_cast<DX12Pipeline*>(pipeline.impl.get());
      bool needsCompile = false;
      if (ptr->pipeline.Get() == nullptr)
      {
        needsCompile = true;
      }
      // TODO: add shader hotreload here. Remember to take the pipelines to safe keeping.
      if (needsCompile || pipeline.m_update->updated())
      {
        m_trash->pipelines.emplace_back(ptr->pipeline);
        m_trash->roots.emplace_back(ptr->root);
        if (pipeline.m_update->updated())
        {
          F_LOG("Updating Compute pipeline %s", pipeline.descriptor.shaderSourcePath.c_str());
        }

        auto thing = m_shaders.shader(pipeline.descriptor.shaderSourcePath, DX12ShaderStorage::ShaderType::Compute, pipeline.descriptor.shaderGroups);
        FAZE_CHECK_HR(m_device->CreateRootSignature(m_nodeMask, thing.data(), thing.size(), IID_PPV_ARGS(&ptr->root)));

        D3D12_SHADER_BYTECODE byte;
        byte.pShaderBytecode = thing.data();
        byte.BytecodeLength = thing.size();

        if (pipeline.m_update->empty())
        {
          *pipeline.m_update = m_shaders.watch(pipeline.descriptor.shaderSourcePath, DX12ShaderStorage::ShaderType::Compute);
        }
        pipeline.m_update->react();

        D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc;
        ZeroMemory(&computeDesc, sizeof(computeDesc));
        computeDesc.CS = byte;
        computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        computeDesc.NodeMask = 0;
        computeDesc.pRootSignature = ptr->root.Get();

        FAZE_CHECK_HR(m_device->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&ptr->pipeline)));
      }
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

      ComPtr<ID3D12Heap> heap;
      m_device->CreateHeap(&dxdesc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf()));
      return GpuHeap(std::make_shared<DX12Heap>(heap), std::move(heapDesc));
    }

    void DX12Device::destroyHeap(GpuHeap)
    {
      //auto native = std::static_pointer_cast<DX12Heap>(heap.impl);
      //native->native()->Release();
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
      DX12ResourceState state;
      state.flags.emplace_back(startState);
      ID3D12Resource* buffer;
      m_device->CreatePlacedResource(native->native(), allocation.allocation.block.offset, &dxDesc, startState, nullptr, IID_PPV_ARGS(&buffer));

      auto wstr = s2ws(desc.desc.name);
      buffer->SetName(wstr.c_str());

      std::weak_ptr<Garbage> weak = m_trash;

      return std::shared_ptr<DX12Buffer>(new DX12Buffer(buffer, std::make_shared<DX12ResourceState>(state)),
        [weak](DX12Buffer* ptr)
      {
        if (auto trash = weak.lock())
        {
          trash->resources.emplace_back(ptr->native());
        }
        else
        {
          ptr->native()->Release();
        }
        delete ptr;
      });
    }

    void DX12Device::destroyBuffer(std::shared_ptr<prototypes::BufferImpl>)
    {
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
        m_device->CreateShaderResourceView(native->native(), &natDesc, descriptor.cpu);
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
        m_device->CreateUnorderedAccessView(native->native(), nullptr, &natDesc, descriptor.cpu);
      }

      std::weak_ptr<Garbage> weak = m_trash;

      return std::shared_ptr<DX12BufferView>(new DX12BufferView(descriptor),
        [weak](DX12BufferView* ptr)
      {
        if (auto trash = weak.lock())
        {
          trash->genericDescriptors.emplace_back(ptr->native());
        }
        delete ptr;
      });
    }

    void DX12Device::destroyBufferView(std::shared_ptr<prototypes::BufferViewImpl>)
    {
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

      DX12ResourceState state;
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
      m_device->CreatePlacedResource(native->native(), allocation.allocation.block.offset, &dxDesc, startState, clearPtr, IID_PPV_ARGS(&texture));

      auto wstr = s2ws(desc.desc.name);
      texture->SetName(wstr.c_str());

      std::weak_ptr<Garbage> weak = m_trash;

      return std::shared_ptr<DX12Texture>(new DX12Texture(texture, std::make_shared<DX12ResourceState>(state)),
        [weak](DX12Texture* ptr)
      {
        if (auto trash = weak.lock())
        {
          trash->resources.emplace_back(ptr->native());
        }
        else
        {
          ptr->native()->Release();
        }
        delete ptr;
      });
    }

    void DX12Device::destroyTexture(std::shared_ptr<prototypes::TextureImpl>)
    {
    }

    std::shared_ptr<prototypes::TextureViewImpl> DX12Device::createTextureView(
      std::shared_ptr<prototypes::TextureImpl> texture, ResourceDescriptor& texDesc, ShaderViewDescriptor& viewDesc)
    {
      auto native = std::static_pointer_cast<DX12Texture>(texture);

      DX12CPUDescriptor descriptor{};

      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        descriptor = m_generics.allocate();
        auto natDesc = dx12::getSRV(texDesc, viewDesc);
        m_device->CreateShaderResourceView(native->native(), &natDesc, descriptor.cpu);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::ReadWrite)
      {
        descriptor = m_generics.allocate();
        auto natDesc = dx12::getUAV(texDesc, viewDesc);
        m_device->CreateUnorderedAccessView(native->native(), nullptr, &natDesc, descriptor.cpu);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::RenderTarget)
      {
        descriptor = m_rtvs.allocate();
        auto natDesc = dx12::getRTV(texDesc, viewDesc);
        m_device->CreateRenderTargetView(native->native(), &natDesc, descriptor.cpu);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::DepthStencil)
      {
        descriptor = m_dsvs.allocate();
        auto natDesc = dx12::getDSV(texDesc, viewDesc);
        m_device->CreateDepthStencilView(native->native(), &natDesc, descriptor.cpu);
      }

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

      std::weak_ptr<Garbage> weak = m_trash;
      auto type = descriptor.type;

      return std::shared_ptr<DX12TextureView>(new DX12TextureView(descriptor, range),
        [weak, type](DX12TextureView* ptr)
      {
        if (auto trash = weak.lock())
        {
          switch (type)
          {
          case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
          {
            trash->rtvsDescriptors.emplace_back(ptr->native());
            break;
          }
          case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
          {
            trash->dsvsDescriptors.emplace_back(ptr->native());
            break;
          }
          default:
          {
            trash->genericDescriptors.emplace_back(ptr->native());
            break;
          }
          }
        }
        delete ptr;
      });
    }

    void DX12Device::destroyTextureView(std::shared_ptr<prototypes::TextureViewImpl>)
    {
    }

    std::shared_ptr<prototypes::DynamicBufferViewImpl> DX12Device::dynamic(MemView<uint8_t> view, FormatType type)
    {
      auto descriptor = m_generics.allocate();
      auto upload = m_dynamicUpload->allocate(view.size());
      F_ASSERT(upload, "Halp");
      memcpy(upload.data(), view.data(), view.size());

      auto format = formatTodxFormat(type).view;
      auto stride = formatSizeInfo(type).pixelSize;

      D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
      desc.Format = format;
      desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      desc.Buffer.NumElements = static_cast<unsigned>(view.size() / stride);
      desc.Buffer.FirstElement = upload.block.offset / stride;
      desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      desc.Buffer.StructureByteStride = format == DXGI_FORMAT_UNKNOWN ? stride : 0;
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

      F_ASSERT(upload.block.offset % stride == 0, "oh no");

      m_device->CreateShaderResourceView(m_dynamicUpload->native(), &desc, descriptor.cpu);

      // will be collected promtly
      m_trash->dynamicBuffers.emplace_back(upload);
      m_trash->genericDescriptors.emplace_back(descriptor);

      return std::make_shared<DX12DynamicBufferView>(upload, descriptor, format, stride);
    }
    std::shared_ptr<prototypes::DynamicBufferViewImpl> DX12Device::dynamic(MemView<uint8_t> view, unsigned stride)
    {
      auto descriptor = m_generics.allocate();
      auto upload = m_dynamicUpload->allocate(view.size());
      F_ASSERT(upload, "Halp");
      memcpy(upload.data(), view.data(), view.size());
      D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      desc.Buffer.NumElements = static_cast<unsigned>(view.size() / stride);
      desc.Buffer.FirstElement = upload.block.offset / stride;
      desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      desc.Buffer.StructureByteStride = stride;
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

      F_ASSERT(upload.block.offset % stride == 0, "oh no");

      m_device->CreateShaderResourceView(m_dynamicUpload->native(), &desc, descriptor.cpu);

      // will be collected promtly
      m_trash->dynamicBuffers.emplace_back(upload);
      m_trash->genericDescriptors.emplace_back(descriptor);

      return std::make_shared<DX12DynamicBufferView>(upload, descriptor, DXGI_FORMAT_UNKNOWN, stride);
    }

    std::shared_ptr<prototypes::DynamicBufferViewImpl> DX12Device::dynamicImage(MemView<uint8_t> bytes, unsigned rowPitch)
    {
      auto rows = bytes.size() / rowPitch;

      constexpr auto APIRowPitchAlignmentRequirement = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

      const auto requiredRowPitch = roundUpMultiplePowerOf2(rowPitch, APIRowPitchAlignmentRequirement);
      F_LOG("rowPitch %zu\n", requiredRowPitch);
      const auto requiredTotalSize = rows * requiredRowPitch;

      auto upload = m_dynamicUpload->allocate(requiredTotalSize);
      F_ASSERT(upload, "Halp");
      for (size_t row = 0; row < rows; ++row)
      {
        auto srcPosition = rowPitch * row;
        auto dstPosition = requiredRowPitch * row;
        memcpy(upload.data() + dstPosition, bytes.data() + srcPosition, rowPitch);
      }

      // will be collected promtly
      m_trash->dynamicBuffers.emplace_back(upload);

      return std::make_shared<DX12DynamicBufferView>(upload, requiredRowPitch);
    }

    DX12CommandBuffer DX12Device::createList(D3D12_COMMAND_LIST_TYPE type)
    {
      ComPtr<ID3D12GraphicsCommandList> commandList;
      ComPtr<ID3D12CommandAllocator> commandListAllocator;
      FAZE_CHECK_HR(m_device->CreateCommandAllocator(type, IID_PPV_ARGS(commandListAllocator.ReleaseAndGetAddressOf())));
      FAZE_CHECK_HR(m_device->CreateCommandList(1, type, commandListAllocator.Get(), NULL, IID_PPV_ARGS(commandList.GetAddressOf())));

      return DX12CommandBuffer(commandList, commandListAllocator);
    }

    DX12Fence DX12Device::createNativeFence()
    {
      ComPtr<ID3D12Fence> fence;
      FAZE_CHECK_HR(m_device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
      return DX12Fence(fence);
    }

    // commandlist things and gpu-cpu/gpu-gpu synchronization primitives
    std::shared_ptr<CommandBufferImpl> DX12Device::createDMAList()
    {
      auto seqNumber = m_seqTracker->next();
      std::weak_ptr<SequenceTracker> tracker = m_seqTracker;
      auto list = std::shared_ptr<DX12CommandList>(new DX12CommandList(m_copyListPool.allocate(), m_constantsUpload, m_dynamicUpload, m_dynamicReadback, m_dynamicGpuDescriptors, m_nullBufferUAV, m_nullBufferSRV), [tracker, seqNumber](DX12CommandList* ptr)
      {
        if (auto seqTracker = tracker.lock())
        {
          seqTracker->complete(seqNumber);
        }
        delete ptr;
      });

      return list;
    }
    std::shared_ptr<CommandBufferImpl> DX12Device::createComputeList()
    {
      auto seqNumber = m_seqTracker->next();
      std::weak_ptr<SequenceTracker> tracker = m_seqTracker;
      auto list = std::shared_ptr<DX12CommandList>(new DX12CommandList(m_computeListPool.allocate(), m_constantsUpload, m_dynamicUpload, m_dynamicReadback, m_dynamicGpuDescriptors, m_nullBufferUAV, m_nullBufferSRV), [tracker, seqNumber](DX12CommandList* ptr)
      {
        if (auto seqTracker = tracker.lock())
        {
          seqTracker->complete(seqNumber);
        }
        delete ptr;
      });
      return list;
    }
    std::shared_ptr<CommandBufferImpl> DX12Device::createGraphicsList()
    {
      auto seqNumber = m_seqTracker->next();
      std::weak_ptr<SequenceTracker> tracker = m_seqTracker;
      auto list = std::shared_ptr<DX12CommandList>(new DX12CommandList(m_graphicsListPool.allocate(), m_constantsUpload, m_dynamicUpload, m_dynamicReadback, m_dynamicGpuDescriptors, m_nullBufferUAV, m_nullBufferSRV), [tracker, seqNumber](DX12CommandList* ptr)
      {
        if (auto seqTracker = tracker.lock())
        {
          seqTracker->complete(seqNumber);
        }
        delete ptr;
      });
      return list;
    }

    std::shared_ptr<SemaphoreImpl> DX12Device::createSemaphore()
    {
      return m_fencePool.allocate();
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
          auto native = std::static_pointer_cast<DX12Fence>(sema);
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
            F_ASSERT(false, "Remove when you feel like it.");
        }
      }
      if (!natList.empty())
        queue->ExecuteCommandLists(static_cast<UINT>(natList.size()), natList.data());

      if (!signal.empty())
      {
        for (auto&& sema : signal)
        {
          auto native = std::static_pointer_cast<DX12Fence>(sema);
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
        auto native = std::static_pointer_cast<DX12Fence>(renderingFinished);
        m_graphicsQueue->Wait(native->fence.Get(), *native->value);
      }
      auto native = std::static_pointer_cast<DX12Swapchain>(swapchain);
      unsigned syncInterval = (native->getDesc().descriptor.desc.mode == PresentMode::Fifo) ? 1 : 0;
      FAZE_CHECK_HR(native->native()->Present(syncInterval, 0));
    }
  }
}
#endif