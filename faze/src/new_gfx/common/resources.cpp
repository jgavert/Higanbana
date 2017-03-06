#include "resources.hpp"
#include "gpudevice.hpp"
#include "graphicssurface.hpp"
//#include "prototypes.hpp"
#include "implementation.hpp"

#include "core/src/math/utils.hpp"

namespace faze
{
  namespace backend
  {
    SubsystemData::SubsystemData(GraphicsApi api, const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : impl(std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion))
      , appName(appName)
      , appVersion(appVersion)
      , engineName(engineName)
      , engineVersion(engineVersion)
    {
      switch (api)
      {
#if defined(FAZE_PLATFORM_WINDOWS)
      case GraphicsApi::DX12:
        impl = std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion);
        break;
#endif
      default:
        impl = std::make_shared<VulkanSubsystem>(appName, appVersion, engineName, engineVersion);
        break;
      }
    }
    std::string SubsystemData::gfxApi() { return impl->gfxApi(); }
    vector<GpuInfo> SubsystemData::availableGpus() { return impl->availableGpus(); }
    GpuDevice SubsystemData::createDevice(FileSystem& fs, GpuInfo gpu) { return impl->createGpuDevice(fs, gpu); }
    GraphicsSurface SubsystemData::createSurface(Window& window) { return impl->createSurface(window); }

    // device
    DeviceData::DeviceData(std::shared_ptr<prototypes::DeviceImpl> impl)
      : m_impl(impl)
      , m_trackerbuffers(std::make_shared<ResourceTracker<prototypes::BufferImpl>>())
      , m_trackertextures(std::make_shared<ResourceTracker<prototypes::TextureImpl>>())
      , m_trackerswapchains(std::make_shared<ResourceTracker<prototypes::SwapchainImpl>>())
      , m_trackerbufferViews(std::make_shared<ResourceTracker<prototypes::BufferViewImpl>>())
      , m_trackertextureViews(std::make_shared<ResourceTracker<prototypes::TextureViewImpl>>())
      , m_idGenerator(std::make_shared<std::atomic<int64_t>>())
    {
    }

    void DeviceData::gc()
    {
      // views
      for (auto&& view : m_trackerbufferViews->getResources())
      {
        m_impl->destroyBufferView(view);
      }

      for (auto&& view : m_trackertextureViews->getResources())
      {
        m_impl->destroyTextureView(view);
      }


      // buffers 
      for (auto&& buffer : m_trackerbuffers->getResources())
      {
        m_impl->destroyBuffer(buffer);
      }

      for (auto&& allocation : m_trackerbuffers->getAllocations())
      {
        m_heaps.release(allocation);
      }
      // textures
      for (auto&& texture : m_trackertextures->getResources())
      {
        m_impl->destroyTexture(texture);
      }

      for (auto&& allocation : m_trackertextures->getAllocations())
      {
        m_heaps.release(allocation);
      }

      for (auto&& sc : m_trackerswapchains->getResources())
      {
        m_impl->destroySwapchain(sc);
      }
    }

    DeviceData::~DeviceData()
    {
      if (m_impl)
      {
        waitGpuIdle();
        gc();
        // heaps
        auto heaps = m_heaps.emptyHeaps();
        for (auto&& heap : heaps)
        {
          m_impl->destroyHeap(heap);
        }
      }
    }

    void DeviceData::waitGpuIdle() { m_impl->waitGpuIdle(); }

    Swapchain DeviceData::createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int bufferCount)
    {
      auto sc = m_impl->createSwapchain(surface, mode, format, bufferCount);
      auto tracker = m_trackerswapchains->makeTracker(newId(), sc);
      auto swapchain = Swapchain(sc, tracker);
      // get backbuffers to swapchain
      auto buffers = m_impl->getSwapchainTextures(swapchain.impl());
      vector<TextureRTV> backbuffers;
      backbuffers.resize(buffers.size());
      for (int i = 0; i < static_cast<int>(buffers.size()); ++i)
      {
        auto texTracker = m_trackertextures->makeTracker(newId(), buffers[i]);
        auto tex = Texture(buffers[i], texTracker, swapchain.impl()->desc());
        backbuffers[i] = createTextureRTV(tex, ShaderViewDescriptor());

      }
      swapchain.setBackbuffers(backbuffers);
      return swapchain;
    }

    void DeviceData::adjustSwapchain(Swapchain& swapchain, PresentMode mode, FormatType format, int bufferCount)
    {
      // stop gpu
      waitGpuIdle();
      // wait all idle work.
      // release current swapchain backbuffers
      swapchain.setBackbuffers({}); // technically this frees the textures if they are not used anywhere.
      // go collect the trash.
      gc();

      // blim
      m_impl->adjustSwapchain(swapchain.impl(), mode, format, bufferCount);
      // get new backbuffers... seems like we do it here.
      auto buffers = m_impl->getSwapchainTextures(swapchain.impl());
      vector<TextureRTV> backbuffers;
      backbuffers.resize(buffers.size());
      for (int i = 0; i < static_cast<int>(buffers.size()); ++i)
      {
        auto texTracker = m_trackertextures->makeTracker(newId(), buffers[i]);
        auto tex = Texture(buffers[i], texTracker, swapchain.impl()->desc());
        backbuffers[i] = createTextureRTV(tex, ShaderViewDescriptor());
      }
      swapchain.setBackbuffers(backbuffers);
      // ...
      // profit!
    }

    TextureRTV DeviceData::acquirePresentableImage(Swapchain& swapchain)
    {
      int index = m_impl->acquirePresentableImage(swapchain.impl());
      return swapchain.buffers()[index];
    }

    Buffer DeviceData::createBuffer(ResourceDescriptor desc)
    {
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createBuffer(allo, desc);
      auto tracker = m_trackerbuffers->makeTracker(newId(), allo.allocation, data);
      return Buffer(data, tracker, desc);
    }

    Texture DeviceData::createTexture(ResourceDescriptor desc)
    {
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createTexture(allo, desc);
      auto tracker = m_trackertextures->makeTracker(newId(), allo.allocation, data);
      return Texture(data, tracker, desc);
    }

    BufferSRV DeviceData::createBufferSRV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createBufferView(buffer.native(), buffer.desc(), viewDesc);
      auto tracker = m_trackerbufferViews->makeTracker(newId(), data);
      return BufferSRV(buffer, data, tracker);
    }

    BufferUAV DeviceData::createBufferUAV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createBufferView(buffer.native(), buffer.desc(), viewDesc);
      auto tracker = m_trackerbufferViews->makeTracker(newId(), data);
      return BufferUAV(buffer, data, tracker);
    }

    TextureSRV DeviceData::createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc);
      auto tracker = m_trackertextureViews->makeTracker(newId(), data);
      return TextureSRV(texture, data, tracker);
    }

    TextureUAV DeviceData::createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc);
      auto tracker = m_trackertextureViews->makeTracker(newId(), data);
      return TextureUAV(texture, data, tracker);
    }

    TextureRTV DeviceData::createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc);
      auto tracker = m_trackertextureViews->makeTracker(newId(), data);
      return TextureRTV(texture, data, tracker);
    }

    TextureDSV DeviceData::createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc);
      auto tracker = m_trackertextureViews->makeTracker(newId(), data);
      return TextureDSV(texture, data, tracker);
    }

    void DeviceData::submit(Swapchain& , CommandGraph graph)
    {
      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        auto& firstList = nodes[0];
        for (int i = 1; i < nodes.size(); ++i)
        {
          firstList.list.list.append(std::move(nodes[i].list.list));
        }

        auto nativeList = m_impl->createGraphicsList();
        nativeList->fillWith(firstList.list.list);
      }
      
    }

    void DeviceData::present(Swapchain& swapchain)
    {
      m_impl->present(swapchain.impl(), swapchain.impl()->acquireSemaphore());
    }

    HeapAllocation HeapManager::allocate(prototypes::DeviceImpl* device, MemoryRequirements requirements)
    {
      GpuHeapAllocation alloc{};
      alloc.alignment = static_cast<int>(requirements.alignment);
      alloc.heapType = requirements.heapType;
      auto createHeapBlock = [&](prototypes::DeviceImpl* dev, int index, MemoryRequirements& requirements)
      {
        auto minSize = std::min(m_minimumHeapSize, static_cast<int64_t>(128 * 32 * requirements.alignment));
        auto sizeToCreate = roundUpMultiple2(minSize, requirements.alignment);
        if (requirements.bytes > sizeToCreate)
        {
          sizeToCreate = roundUpMultiple2(requirements.bytes, requirements.alignment);
        }
        std::string name;
        name += std::to_string(index);
        name += "Heap";
        name += std::to_string(static_cast<int>(requirements.heapType));
        name += "a";
        name += std::to_string(requirements.alignment);
        HeapDescriptor desc = HeapDescriptor()
          .setSizeInBytes(sizeToCreate)
          .setHeapType(HeapType::Custom)
          .setHeapTypeSpecific(requirements.heapType)
          .setHeapAlignment(requirements.alignment)
          .setName(name);
        F_SLOG("Graphics","Created heap \"%s\" size %zu\n", name.c_str(), sizeToCreate);
        return HeapBlock{ index, FixedSizeAllocator(requirements.alignment, sizeToCreate / requirements.alignment), dev->createHeap(desc) };
      };

      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.alignment == requirements.alignment
            && vec.type      == requirements.heapType;
      });
      if (vectorPtr != m_heaps.end()) // found alignment
      {
        PageBlock block{ -1, -1 };
        for (auto& heap : vectorPtr->heaps)
        {
          if (heap.allocator.freesize() > requirements.bytes) // this will find falsepositives if memory is fragmented.
          {
            block = heap.allocator.allocate(requirements.bytes); // should be cheap anyway
            if (block.valid())
            {
              alloc.block = block;
              alloc.index = heap.index;
              return HeapAllocation{ alloc, heap.heap};
            }
          }
        }
        if (!block.valid())
        {
          // create correct sized heap and allocate from it.
          auto newHeap = createHeapBlock(device, static_cast<int>(vectorPtr->heaps.size()), requirements);
          alloc.block = newHeap.allocator.allocate(requirements.bytes);
          alloc.index = newHeap.index;
          auto heap = newHeap.heap;
          vectorPtr->heaps.emplace_back(std::move(newHeap));
          return HeapAllocation{ alloc, heap };
        }
      }
      HeapVector vec{ static_cast<int>(requirements.alignment), requirements.heapType, vector<HeapBlock>()};
      auto newHeap = createHeapBlock(device, 0, requirements);
      alloc.block = newHeap.allocator.allocate(requirements.bytes);
      alloc.index = newHeap.index;
      auto heap = newHeap.heap;
      vec.heaps.emplace_back(std::move(newHeap));
      m_heaps.emplace_back(std::move(vec));
      return HeapAllocation{ alloc, heap };
    }

    void HeapManager::release(GpuHeapAllocation object)
    {
      F_ASSERT(object.valid(), "invalid object was released");
      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.alignment == object.alignment
            && vec.type == object.heapType;
      });
      if (vectorPtr != m_heaps.end())
      {
        vectorPtr->heaps[object.index].allocator.release(object.block);
      }
    }

    vector<GpuHeap> HeapManager::emptyHeaps()
    {
      vector<GpuHeap> emptyHeaps;
      for (auto&& it : m_heaps)
      {
        for (auto&& heap : it.heaps)
        {
          if (heap.allocator.empty())
          {
            emptyHeaps.emplace_back(heap.heap);
          }
        }
      }
      return emptyHeaps;
    }
  }
}

