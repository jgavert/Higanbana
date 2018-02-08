#include "resources.hpp"
#include "gpudevice.hpp"
#include "graphicssurface.hpp"
#include "implementation.hpp"
#include "semaphore.hpp"
#include "faze/src/new_gfx/common/cpuimage.hpp"

#include "core/src/math/utils.hpp"

namespace faze
{
  namespace backend
  {
    SubsystemData::SubsystemData(GraphicsApi api, const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : impl(nullptr)
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
      , m_idGenerator(std::make_shared<std::atomic<int64_t>>())
    {
    }

    void DeviceData::checkCompletedLists()
    {
      if (!m_buffers.empty())
      {
        int buffersToFree = 0;
        int buffersWithoutFence = 0;
        int bufferCount = static_cast<int>(m_buffers.size());
        for (int i = 0; i < bufferCount; ++i)
        {
          if (m_buffers[i].fence)
          {
            if (!m_impl->checkFence(m_buffers[i].fence))
            {
              break;
            }
            buffersToFree += buffersWithoutFence + 1;
            buffersWithoutFence = 0;
          }
          else
          {
            buffersWithoutFence++;
          }
        }

        while (buffersToFree > 0)
        {
          m_buffers.pop_front();
          buffersToFree--;
        }
      }
    }

    void DeviceData::gc()
    {
      checkCompletedLists();

      for (auto&& allocation : m_trackerbuffers->getAllocations())
      {
        m_heaps.release(allocation);
      }

      for (auto&& allocation : m_trackertextures->getAllocations())
      {
        m_heaps.release(allocation);
      }

      m_heaps.emptyHeaps();

      garbageCollection();
    }

    DeviceData::~DeviceData()
    {
      if (m_impl)
      {
        m_impl->waitGpuIdle();
        waitGpuIdle();
        gc();
      }
    }

    void DeviceData::waitGpuIdle()
    {
      if (!m_buffers.empty())
      {
        for (auto&& liveBuffer : m_buffers)
        {
          if (liveBuffer.fence)
          {
            m_impl->waitFence(liveBuffer.fence);
          }
        }

        while (!m_buffers.empty())
        {
          m_buffers.pop_front();
        }
      }
    }

    Swapchain DeviceData::createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor)
    {
      auto sc = m_impl->createSwapchain(surface, descriptor);
      auto swapchain = Swapchain(sc);
      // get backbuffers to swapchain
      auto buffers = m_impl->getSwapchainTextures(swapchain.impl());
      vector<TextureRTV> backbuffers;
      backbuffers.resize(buffers.size());
      for (int i = 0; i < static_cast<int>(buffers.size()); ++i)
      {
        auto tex = Texture(buffers[i], std::make_shared<int64_t>(newId()), std::make_shared<ResourceDescriptor>(swapchain.impl()->desc()));
        backbuffers[i] = createTextureRTV(tex, ShaderViewDescriptor());
      }
      swapchain.setBackbuffers(backbuffers);
      return swapchain;
    }

    void DeviceData::adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor)
    {
      // stop gpu
      waitGpuIdle();
      // wait all idle work.
      // release current swapchain backbuffers
      swapchain.setBackbuffers({}); // technically this frees the textures if they are not used anywhere.
      // go collect the trash.
      gc();

      // blim
      m_impl->adjustSwapchain(swapchain.impl(), descriptor);
      // get new backbuffers... seems like we do it here.
      auto buffers = m_impl->getSwapchainTextures(swapchain.impl());
      vector<TextureRTV> backbuffers;
      backbuffers.resize(buffers.size());
      for (int i = 0; i < static_cast<int>(buffers.size()); ++i)
      {
        auto tex = Texture(buffers[i], std::make_shared<int64_t>(newId()), std::make_shared<ResourceDescriptor>(swapchain.impl()->desc()));
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

    TextureRTV* DeviceData::tryAcquirePresentableImage(Swapchain& swapchain)
    {
        int index = m_impl->tryAcquirePresentableImage(swapchain.impl());
        if (index == -1)
            return nullptr;
        return &(swapchain.buffers()[index]);
    }

    Renderpass DeviceData::createRenderpass()
    {
      return Renderpass(m_impl->createRenderpass());
    }

    ComputePipeline DeviceData::createComputePipeline(ComputePipelineDescriptor desc)
    {
      return ComputePipeline(m_impl->createPipeline(), desc);
    }

    GraphicsPipeline DeviceData::createGraphicsPipeline(GraphicsPipelineDescriptor desc)
    {
      return GraphicsPipeline(desc);
    }

    Buffer DeviceData::createBuffer(ResourceDescriptor desc)
    {
      desc = desc.setDimension(FormatDimension::Buffer);
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createBuffer(allo, desc);
      auto tracker = m_trackerbuffers->makeTracker(newId(), allo.allocation);
      return Buffer(data, tracker, std::make_shared<ResourceDescriptor>(std::move(desc)));
    }

    SharedBuffer DeviceData::createSharedBuffer(DeviceData& secondary, ResourceDescriptor desc)
    {
      desc = desc.setDimension(FormatDimension::Buffer);
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createBuffer(allo, desc);
      auto handle = m_impl->openSharedHandle(allo);
      auto secondaryData = secondary.m_impl->createBufferFromHandle(handle, allo, desc);
      auto tracker = m_trackerbuffers->makeTracker(newId(), allo.allocation);
      return SharedBuffer(data, secondaryData, tracker, std::make_shared<ResourceDescriptor>(std::move(desc)));
    }

    Texture DeviceData::createTexture(ResourceDescriptor desc)
    {
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createTexture(allo, desc);
      auto tracker = m_trackertextures->makeTracker(newId(), allo.allocation);
      return Texture(data, tracker, std::make_shared<ResourceDescriptor>(desc));
    }

    SharedTexture DeviceData::createSharedTexture(DeviceData& secondary, ResourceDescriptor desc)
    {
      auto data = m_impl->createTexture(desc);
      auto handle = m_impl->openSharedHandle(data);
      auto secondaryData = secondary.m_impl->createTextureFromHandle(handle, desc);
      return SharedTexture(data, secondaryData, std::make_shared<int64_t>(newId()), std::make_shared<ResourceDescriptor>(std::move(desc)));
    }

    BufferSRV DeviceData::createBufferSRV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createBufferView(buffer.native(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      return BufferSRV(buffer, data);
    }

    BufferUAV DeviceData::createBufferUAV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createBufferView(buffer.native(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      return BufferUAV(buffer, data);
    }

    SubresourceRange rangeFrom(const ShaderViewDescriptor& view, const ResourceDescriptor& desc, bool srv)
    {
      SubresourceRange range{};
      range.mipOffset = static_cast<int16_t>(view.m_mostDetailedMip);
      range.mipLevels = static_cast<int16_t>(view.m_mipLevels);
      range.sliceOffset = static_cast<int16_t>(view.m_arraySlice);
      range.arraySize = static_cast<int16_t>(view.m_arraySize);

      if (range.mipLevels == -1)
      {
        range.mipLevels = static_cast<int16_t>(desc.desc.miplevels - range.mipOffset);
      }

      if (range.arraySize == -1)
      {
        range.arraySize = static_cast<int16_t>(desc.desc.arraySize - range.sliceOffset);
        if (desc.desc.dimension == FormatDimension::TextureCube)
        {
          range.arraySize = static_cast<int16_t>((desc.desc.arraySize * 6) - range.sliceOffset);
        }
      }

      if (!srv)
      {
        range.mipLevels = 1;
      }

      return range;
    }

    TextureSRV DeviceData::createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      return TextureSRV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), true), viewDesc.m_format);
    }

    TextureUAV DeviceData::createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      return TextureUAV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), false), viewDesc.m_format);
    }

    TextureRTV DeviceData::createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::RenderTarget));
      return TextureRTV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), false), viewDesc.m_format);
    }

    TextureDSV DeviceData::createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::DepthStencil));
      return TextureDSV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), false), viewDesc.m_format);
    }

    DynamicBufferView DeviceData::dynamicBuffer(MemView<uint8_t> view, FormatType type)
    {
      return m_impl->dynamic(view, type);
    }

    DynamicBufferView DeviceData::dynamicBuffer(MemView<uint8_t> view, unsigned stride)
    {
      return m_impl->dynamic(view, stride);
    }

    bool DeviceData::uploadInitialTexture(Texture& tex, CpuImage& image)
    {
      auto arraySize = image.desc().desc.arraySize;
      for (auto slice = 0u; slice < arraySize; ++slice)
      {
        CommandList list;

        for (auto mip = 0u; mip < image.desc().desc.miplevels; ++mip)
        {
          auto sr = image.subresource(mip, slice);
          DynamicBufferView dynBuffer = m_impl->dynamicImage(makeByteView(sr.data(), sr.size()), static_cast<unsigned>(sr.rowPitch()));
          list.updateTexture(tex, dynBuffer, mip, slice);
        }
        unordered_set<TrackedState> lols;
        lols.insert(tex.dependency());
        list.prepareForQueueSwitch(lols);

        // just submit the list
        auto nativeList = m_impl->createDMAList();
        nativeList->fillWith(m_impl, list.list);
        LiveCommandBuffer buffer{};
        buffer.lists.emplace_back(nativeList);

        buffer.fence = m_impl->createFence();
        buffer.intermediateLists = std::make_shared<vector<IntermediateList>>();
        buffer.intermediateLists->emplace_back(std::move(list.list));

        m_impl->submitDMA(buffer.lists, buffer.wait, buffer.signal, buffer.fence);
        m_buffers.emplace_back(buffer);
      }
      return true;
    }

    GpuSemaphore DeviceData::createSemaphore()
    {
      return GpuSemaphore(m_impl->createSemaphore());
    }

    GpuSharedSemaphore DeviceData::createSharedSemaphore(DeviceData& secondaryGpu)
    {
      auto primary = m_impl->createSharedSemaphore();
      auto handle = m_impl->openSharedHandle(primary);
      auto secondary = secondaryGpu.m_impl->createSemaphoreFromHandle(handle);
      return GpuSharedSemaphore(primary, secondary);
    }

    void DeviceData::submit(Swapchain& swapchain, CommandGraph graph)
    {
#if 0 // old
      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        auto& firstList = nodes[0];
        for (int i = 1; i < static_cast<int>(nodes.size()); ++i)
        {
          firstList.list.list.append(std::move(nodes[i].list.list));
        }

        auto nativeList = m_impl->createGraphicsList();
        nativeList->fillWith(m_impl, firstList.list.list);
        LiveCommandBuffer buffer{};
        buffer.lists.emplace_back(nativeList);

        auto waitSema = swapchain.impl()->acquireSemaphore();
        if (waitSema)
        {
          buffer.wait.emplace_back(waitSema);
        }

        auto renderComplete = swapchain.impl()->renderSemaphore();
        if (renderComplete)
        {
          buffer.signal.emplace_back(renderComplete);
        }

        buffer.fence = m_impl->createFence();
        buffer.intermediateLists = std::make_shared<vector<IntermediateList>>();
        buffer.intermediateLists->emplace_back(std::move(firstList.list.list));

        m_impl->submitGraphics(buffer.lists, buffer.wait, buffer.signal, buffer.fence);
        m_buffers.emplace_back(buffer);
      }
#else

      struct PreparedCommandlist
      {
        CommandGraphNode::NodeType type;
        CommandList list;
        unordered_set<backend::TrackedState> requirements;
        std::shared_ptr<SemaphoreImpl> sema;
        bool presents = false;
        bool isLastList = false;
      };

      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        int i = 0;

        vector<PreparedCommandlist> lists;

        while (i < static_cast<int>(nodes.size()))
        {
          auto* firstList = &nodes[i];

          PreparedCommandlist plist{};
          plist.type = firstList->type;
          plist.list = std::move(nodes[i].list);
          plist.requirements = nodes[i].needsResources();
          plist.sema = nodes[i].acquireSemaphore;
          plist.presents = nodes[i].preparesPresent;

          i += 1;
          for (; i < static_cast<int>(nodes.size()); ++i)
          {
            if (nodes[i].type != plist.type)
              break;
            plist.list.list.append(std::move(nodes[i].list.list));
            const auto& ref = nodes[i].needsResources();
            plist.requirements.insert(ref.begin(), ref.end());
            if (!plist.sema)
            {
              plist.sema = nodes[i].acquireSemaphore;
            }
            if (!plist.presents)
            {
              plist.presents = nodes[i].preparesPresent;
            }
          }

          lists.emplace_back(std::move(plist));
        }

        lists[lists.size() - 1].isLastList = true;

        std::shared_ptr<SemaphoreImpl> optionalWaitSemaphore;

        for (auto&& list : lists)
        {
          if (!list.requirements.empty())
          {
            list.list.prepareForQueueSwitch(list.requirements);
          }

          std::shared_ptr<CommandBufferImpl> nativeList;

          switch (list.type)
          {
          case CommandGraphNode::NodeType::DMA:
            nativeList = m_impl->createDMAList();
            break;
          case CommandGraphNode::NodeType::Compute:
            nativeList = m_impl->createComputeList();
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            nativeList = m_impl->createGraphicsList();
          }

          nativeList->fillWith(m_impl, list.list.list);

          LiveCommandBuffer buffer{};
          buffer.lists.emplace_back(nativeList);

          buffer.intermediateLists = std::make_shared<vector<IntermediateList>>();
          buffer.intermediateLists->emplace_back(std::move(list.list.list));

          if (optionalWaitSemaphore)
          {
            buffer.wait.push_back(optionalWaitSemaphore);
          }

          if (list.sema)
          {
            buffer.wait.push_back(list.sema);
          }

          if (!list.isLastList)
          {
            optionalWaitSemaphore = m_impl->createSemaphore();
            buffer.signal.push_back(optionalWaitSemaphore);
          }

          if (list.presents)
          {
            auto presenene = swapchain.impl()->renderSemaphore();
            if (presenene)
            {
              buffer.signal.push_back(presenene);
            }
          }

          if (list.isLastList)
          {
            buffer.fence = m_impl->createFence();
          }

          MemView<std::shared_ptr<FenceImpl>> viewToFences;

          if (buffer.fence)
          {
            viewToFences = buffer.fence;
          }

          switch (list.type)
          {
          case CommandGraphNode::NodeType::DMA:
            m_impl->submitDMA(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Compute:
            m_impl->submitCompute(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            m_impl->submitGraphics(buffer.lists, buffer.wait, buffer.signal, viewToFences);
          }
          m_buffers.emplace_back(buffer);
        }
      }
#endif

      gc();
    }

    void DeviceData::submit(CommandGraph graph)
    {
#if 0 // old
      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        auto& firstList = nodes[0];
        for (int i = 1; i < static_cast<int>(nodes.size()); ++i)
        {
          firstList.list.list.append(std::move(nodes[i].list.list));
        }

        auto nativeList = m_impl->createGraphicsList();
        nativeList->fillWith(m_impl, firstList.list.list);
        LiveCommandBuffer buffer{};
        buffer.lists.emplace_back(nativeList);

        buffer.fence = m_impl->createFence();
        buffer.intermediateLists = std::make_shared<vector<IntermediateList>>();
        buffer.intermediateLists->emplace_back(std::move(firstList.list.list));

        m_impl->submitGraphics(buffer.lists, buffer.wait, buffer.signal, buffer.fence);
        m_buffers.emplace_back(buffer);
      }
#elif 0
      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        int i = 0;

        std::shared_ptr<SemaphoreImpl> optionalWaitSemaphore;

        while (i < nodes.size())
        {
          auto* firstList = &nodes[i];
          auto currentListType = firstList->type;

          i += 1;
          for (; i < static_cast<int>(nodes.size()); ++i)
          {
            if (nodes[i].type != currentListType)
              break;
            firstList->list.list.append(std::move(nodes[i].list.list));
          }

          bool isLastList = i >= nodes.size();

          std::shared_ptr<CommandBufferImpl> nativeList;

          switch (currentListType)
          {
          case CommandGraphNode::NodeType::DMA:
            nativeList = m_impl->createDMAList();
            break;
          case CommandGraphNode::NodeType::Compute:
            nativeList = m_impl->createComputeList();
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            nativeList = m_impl->createGraphicsList();
          }

          nativeList->fillWith(m_impl, firstList->list.list);

          LiveCommandBuffer buffer{};
          buffer.lists.emplace_back(nativeList);

          buffer.intermediateLists = std::make_shared<vector<IntermediateList>>();
          buffer.intermediateLists->emplace_back(std::move(firstList->list.list));

          if (optionalWaitSemaphore)
          {
            buffer.wait.push_back(optionalWaitSemaphore);
          }

          if (!isLastList)
          {
            optionalWaitSemaphore = m_impl->createSemaphore();
            buffer.signal.push_back(optionalWaitSemaphore);
          }

          if (isLastList)
          {
            buffer.fence = m_impl->createFence();
          }

          MemView<std::shared_ptr<FenceImpl>> viewToFences;

          if (buffer.fence)
          {
            viewToFences = buffer.fence;
          }

          switch (currentListType)
          {
          case CommandGraphNode::NodeType::DMA:
            m_impl->submitDMA(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Compute:
            m_impl->submitCompute(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            m_impl->submitGraphics(buffer.lists, buffer.wait, buffer.signal, viewToFences);
          }
          m_buffers.emplace_back(buffer);
        }
      }
#else

      struct PreparedCommandlist
      {
        CommandGraphNode::NodeType type;
        CommandList list;
        unordered_set<backend::TrackedState> requirements;
        bool isLastList = false;
      };

      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        int i = 0;

        vector<PreparedCommandlist> lists;

        while (i < static_cast<int>(nodes.size()))
        {
          auto* firstList = &nodes[i];

          PreparedCommandlist plist{};
          plist.type = firstList->type;
          plist.list = std::move(nodes[i].list);
          plist.requirements = nodes[i].needsResources();

          i += 1;
          for (; i < static_cast<int>(nodes.size()); ++i)
          {
            if (nodes[i].type != plist.type)
              break;
            plist.list.list.append(std::move(nodes[i].list.list));
            const auto& ref = nodes[i].needsResources();
            plist.requirements.insert(ref.begin(), ref.end());
          }

          lists.emplace_back(std::move(plist));
        }

        lists[lists.size() - 1].isLastList = true;

        std::shared_ptr<SemaphoreImpl> optionalWaitSemaphore;

        for (auto&& list : lists)
        {
          if (!list.requirements.empty())
          {
            list.list.prepareForQueueSwitch(list.requirements);
          }
          std::shared_ptr<CommandBufferImpl> nativeList;

          switch (list.type)
          {
          case CommandGraphNode::NodeType::DMA:
            nativeList = m_impl->createDMAList();
            break;
          case CommandGraphNode::NodeType::Compute:
            nativeList = m_impl->createComputeList();
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            nativeList = m_impl->createGraphicsList();
          }

          nativeList->fillWith(m_impl, list.list.list);

          LiveCommandBuffer buffer{};
          buffer.lists.emplace_back(nativeList);

          buffer.intermediateLists = std::make_shared<vector<IntermediateList>>();
          buffer.intermediateLists->emplace_back(std::move(list.list.list));

          if (optionalWaitSemaphore)
          {
            buffer.wait.push_back(optionalWaitSemaphore);
          }

          if (!list.isLastList)
          {
            optionalWaitSemaphore = m_impl->createSemaphore();
            buffer.signal.push_back(optionalWaitSemaphore);
          }

          if (list.isLastList)
          {
            buffer.fence = m_impl->createFence();
          }

          MemView<std::shared_ptr<FenceImpl>> viewToFences;

          if (buffer.fence)
          {
            viewToFences = buffer.fence;
          }

          switch (list.type)
          {
          case CommandGraphNode::NodeType::DMA:
            m_impl->submitDMA(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Compute:
            m_impl->submitCompute(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            m_impl->submitGraphics(buffer.lists, buffer.wait, buffer.signal, viewToFences);
          }
          m_buffers.emplace_back(buffer);
        }
      }
#endif

      gc();
    }

    void DeviceData::explicitSubmit(CommandGraph graph)
    {
      struct PreparedCommandlist
      {
        CommandGraphNode::NodeType type;
        CommandList list;
        unordered_set<backend::TrackedState> requirements;
        vector<std::shared_ptr<backend::SemaphoreImpl>> wait;
        vector<std::shared_ptr<backend::SemaphoreImpl>> signal;
        bool isLastList = false;
      };

      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        int i = 0;

        vector<PreparedCommandlist> lists;

        while (i < static_cast<int>(nodes.size()))
        {
          auto* firstList = &nodes[i];

          PreparedCommandlist plist{};
          plist.type = firstList->type;
          plist.list = std::move(nodes[i].list);
          plist.requirements = nodes[i].needsResources();
          plist.wait = std::move(nodes[i].m_wait);
          plist.signal = std::move(nodes[i].m_signal);

          i += 1;
          for (; i < static_cast<int>(nodes.size()); ++i)
          {
            if (nodes[i].type != plist.type)
              break;
            if (!nodes[i].m_wait.empty())
              break;
            plist.list.list.append(std::move(nodes[i].list.list));
            const auto& ref = nodes[i].needsResources();
            plist.requirements.insert(ref.begin(), ref.end());
            plist.signal.insert(std::end(plist.signal), std::begin(nodes[i].m_signal), std::end(nodes[i].m_signal));
          }

          lists.emplace_back(std::move(plist));
        }

        lists[lists.size() - 1].isLastList = true;

        for (auto&& list : lists)
        {
          if (!list.requirements.empty())
          {
            list.list.prepareForQueueSwitch(list.requirements);
          }
          std::shared_ptr<CommandBufferImpl> nativeList;

          switch (list.type)
          {
          case CommandGraphNode::NodeType::DMA:
            nativeList = m_impl->createDMAList();
            break;
          case CommandGraphNode::NodeType::Compute:
            nativeList = m_impl->createComputeList();
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            nativeList = m_impl->createGraphicsList();
          }

          nativeList->fillWith(m_impl, list.list.list);

          LiveCommandBuffer buffer{};
          buffer.lists.emplace_back(nativeList);

          buffer.intermediateLists = std::make_shared<vector<IntermediateList>>();
          buffer.intermediateLists->emplace_back(std::move(list.list.list));
          buffer.wait = std::move(list.wait);
          buffer.signal = std::move(list.signal);

          if (list.isLastList)
          {
            buffer.fence = m_impl->createFence();
          }

          MemView<std::shared_ptr<FenceImpl>> viewToFences;

          if (buffer.fence)
          {
            viewToFences = buffer.fence;
          }

          switch (list.type)
          {
          case CommandGraphNode::NodeType::DMA:
            m_impl->submitDMA(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Compute:
            m_impl->submitCompute(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            m_impl->submitGraphics(buffer.lists, buffer.wait, buffer.signal, viewToFences);
          }
          m_buffers.emplace_back(buffer);
        }
      }

      gc();
    }

    void DeviceData::garbageCollection()
    {
      m_impl->collectTrash();
    }

    void DeviceData::present(Swapchain& swapchain)
    {
      m_impl->present(swapchain.impl(), swapchain.impl()->renderSemaphore());
    }

    HeapAllocation HeapManager::allocate(prototypes::DeviceImpl* device, MemoryRequirements requirements)
    {
      GpuHeapAllocation alloc{};
      alloc.alignment = static_cast<int>(requirements.alignment);
      alloc.heapType = requirements.heapType;
      auto createHeapBlock = [&](prototypes::DeviceImpl* dev, uint64_t index, MemoryRequirements& requirements)
      {
        auto minSize = std::min(m_minimumHeapSize, static_cast<int64_t>(128 * 32 * requirements.alignment));
        auto sizeToCreate = roundUpMultiplePowerOf2(minSize, requirements.alignment);
        if (requirements.bytes > sizeToCreate)
        {
          sizeToCreate = roundUpMultiplePowerOf2(requirements.bytes, requirements.alignment);
        }
        std::string name;
        name += std::to_string(index);
        name += "Heap";
        name += std::to_string(requirements.heapType);
        name += "a";
        name += std::to_string(requirements.alignment);

        m_totalMemory += sizeToCreate;

        HeapDescriptor desc = HeapDescriptor()
          .setSizeInBytes(sizeToCreate)
          .setHeapType(HeapType::Custom)
          .setHeapTypeSpecific(requirements.heapType)
          .setHeapAlignment(requirements.alignment)
          .setName(name);
        GFX_LOG("Created heap \"%s\" size %.2fMB (%zu). Total memory in heaps %.2fMB", name.c_str(), float(sizeToCreate) / 1024.f / 1024.f, sizeToCreate, float(m_totalMemory) / 1024.f / 1024.f);
        return HeapBlock{ index, FixedSizeAllocator(requirements.alignment, sizeToCreate / requirements.alignment), dev->createHeap(desc) };
      };

      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.alignment == static_cast<int>(requirements.alignment)
          && vec.type == requirements.heapType;
      });
      if (vectorPtr != m_heaps.end()) // found alignment
      {
        PageBlock block{ -1, -1 };
        for (auto& heap : vectorPtr->heaps)
        {
          if (heap.allocator.freesize() >= requirements.bytes) // this will find falsepositives if memory is fragmented.
          {
            block = heap.allocator.allocate(requirements.bytes); // should be cheap anyway
            if (block.valid())
            {
              alloc.block = block;
              alloc.index = heap.index;
              return HeapAllocation{ alloc, heap.heap };
            }
          }
        }
        if (!block.valid())
        {
          // create correct sized heap and allocate from it.
          auto newHeap = createHeapBlock(device, m_heapIndex++, requirements);
          alloc.block = newHeap.allocator.allocate(requirements.bytes);
          alloc.index = newHeap.index;
          auto heap = newHeap.heap;
          vectorPtr->heaps.emplace_back(std::move(newHeap));
          return HeapAllocation{ alloc, heap };
        }
      }
      HeapVector vec{ static_cast<int>(requirements.alignment), requirements.heapType, vector<HeapBlock>() };
      auto newHeap = createHeapBlock(device, m_heapIndex++, requirements);
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
        auto heapPtr = std::find_if(vectorPtr->heaps.begin(), vectorPtr->heaps.end(), [&](HeapBlock& vec)
        {
          return vec.index == object.index;
        });
        heapPtr->allocator.release(object.block);
      }
    }

    vector<GpuHeap> HeapManager::emptyHeaps()
    {
      vector<GpuHeap> emptyHeaps;
      for (auto&& it : m_heaps)
      {
        for (auto iter = it.heaps.begin(); iter != it.heaps.end(); ++iter)
        {
          if (iter->allocator.empty())
          {
            emptyHeaps.emplace_back(iter->heap);
            m_totalMemory -= iter->allocator.size();
            GFX_LOG("Destroyed heap \"%dHeap%zda%d\" size %zu. Total memory in heaps %.2fMB", iter->index, it.type, it.alignment, iter->allocator.size(), float(m_totalMemory) / 1024.f / 1024.f);
          }
        }
        for (;;)
        {
          auto removables = std::find_if(it.heaps.begin(), it.heaps.end(), [&](HeapBlock& vec)
          {
            return vec.allocator.empty();
          });
          if (removables != it.heaps.end())
          {
            it.heaps.erase(removables);
          }
          else
          {
            break;
          }
        }
      }

      auto removables = std::remove_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.heaps.empty();
      });
      if (removables != m_heaps.end())
      {
        m_heaps.erase(removables);
      }
      return emptyHeaps;
    }
  }
}