#include "graphics/common/device_group_data.hpp"
#include "graphics/common/gpudevice.hpp"
#include "graphics/common/graphicssurface.hpp"
#include "graphics/common/implementation.hpp"
#include "graphics/common/semaphore.hpp"
#include "graphics/common/cpuimage.hpp"
#include <graphics/common/swapchain.hpp>
#include <graphics/common/pipeline.hpp>
#include <graphics/common/commandgraph.hpp>

#include "core/math/utils.hpp"

namespace faze
{
  namespace backend
  {
    // device
    DeviceGroupData::DeviceGroupData(vector<std::shared_ptr<prototypes::DeviceImpl>> impls, vector<GpuInfo> infos)
    {
      // all devices have their own heap managers
      for (int i = 0; i < impls.size(); ++i)
      {
        m_devices.push_back({i, impls[i], HeapManager(), infos[i]});
      }
    }

    void DeviceGroupData::checkCompletedLists()
    {
      if (!m_buffers.empty())
      {
        if (m_buffers.size() > 8) // throttle so that we don't go too far ahead.
        {
          // force wait oldest
          int deviceID = m_buffers.front().deviceID;
          m_devices[deviceID].device->waitFence(m_buffers.front().fence);
        }
        int buffersToFree = 0;
        int buffersWithoutFence = 0;
        int bufferCount = static_cast<int>(m_buffers.size());
        for (int i = 0; i < bufferCount; ++i)
        {
          if (m_buffers[i].fence)
          {
            if (!m_devices[m_buffers[i].deviceID].device->checkFence(m_buffers[i].fence))
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

        auto currentSeqNum = InvalidSeqNum;

        while (buffersToFree > 0)
        {
          auto& buffer = m_buffers.front();
          if (buffer.started != currentSeqNum)
          {
            m_seqTracker.complete(buffer.started);
            currentSeqNum = buffer.started;
          }
          m_buffers.pop_front();
          buffersToFree--;
        }
      }
    }

    void DeviceGroupData::gc()
    {
      checkCompletedLists();
      garbageCollection();
    }

    DeviceGroupData::~DeviceGroupData()
    {
      for (auto&& impl : m_devices)
      {
        if (impl.device)
        {
          impl.device->waitGpuIdle();
          waitGpuIdle();
        }
      }
      gc();
    }

    void DeviceGroupData::waitGpuIdle()
    {
      if (!m_buffers.empty())
      {
        for (auto&& liveBuffer : m_buffers)
        {
          if (liveBuffer.fence)
          {
            m_devices[liveBuffer.deviceID].device->waitFence(liveBuffer.fence);
          }
        }

        while (!m_buffers.empty())
        {
          m_buffers.pop_front();
        }
      }
    }

    constexpr const int SwapchainDeviceID = 0;

    void DeviceGroupData::configureBackbufferViews(Swapchain& sc)
    {
      vector<ResourceHandle> handles;
      // overcommitting with handles, 8 is max anyway...
      for (int i = 0; i < 8; i++)
      {
        handles.push_back(m_handles.allocate(ResourceType::Texture));
      }
      auto bufferCount = m_devices[SwapchainDeviceID].device->fetchSwapchainTextures(sc.impl(), handles);
      vector<TextureRTV> backbuffers;
      backbuffers.resize(bufferCount);
      for (int i = 0; i < bufferCount; ++i)
      {
        std::shared_ptr<ResourceHandle> handlePtr = std::shared_ptr<ResourceHandle>(new ResourceHandle(handles[i]), 
        [weakDevice = weak_from_this()](ResourceHandle* ptr)
        {
          if (auto dev = weakDevice.lock())
          {
            dev->m_handles.release(*ptr);
          }
          delete ptr;
        });

        auto tex = Texture(handlePtr, std::make_shared<ResourceDescriptor>(sc.impl()->desc()));
        auto viewDesc = ShaderViewDescriptor();
        if (viewDesc.m_format == FormatType::Unknown)
        {
          viewDesc.m_format = tex.desc().desc.format;
        }

        auto handle = m_handles.allocate(ResourceType::TextureRTV);
        handle.setGpuId(0);
        m_devices[0].device->createTextureView(handle, tex.handle(), tex.desc(), viewDesc.setType(ResourceShaderType::RenderTarget));
        backbuffers[i] = TextureRTV(tex, sharedHandle(handle));
      }

      sc.setBackbuffers(backbuffers);

      // release overcommitted handles
      for (int i = bufferCount; i < 8; i++)
      {
        m_handles.release(handles[i]);
      }
    }

    Swapchain DeviceGroupData::createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor)
    {
      // assume that first device is the one in charge of presenting. TODO: swapchain is created always on device 0
      auto sc = m_devices[SwapchainDeviceID].device->createSwapchain(surface, descriptor);
      auto swapchain = Swapchain(sc);
      // get backbuffers to swapchain
      configureBackbufferViews(swapchain);
      return swapchain;
    }

    void DeviceGroupData::adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor)
    {
      // stop gpu, possibly wait for last 'present' by inserting only fence to queue.
      auto fenceForSwapchain = m_devices[SwapchainDeviceID].device->createFence();
      // assuming that only graphics queue accesses swapchain resources.
      m_devices[SwapchainDeviceID].device->submitGraphics({}, {}, {}, fenceForSwapchain);
      m_devices[SwapchainDeviceID].device->waitFence(fenceForSwapchain);
      waitGpuIdle();
      // wait all idle work.
      // release current swapchain backbuffers
      swapchain.setBackbuffers({}); // technically this frees the textures if they are not used anywhere.
      // go collect the trash.
      gc();

      // blim
      m_devices[SwapchainDeviceID].device->adjustSwapchain(swapchain.impl(), descriptor);
      // get new backbuffers... seems like we do it here.

      configureBackbufferViews(swapchain);
    }

    TextureRTV DeviceGroupData::acquirePresentableImage(Swapchain& swapchain)
    {
      int index = m_devices[SwapchainDeviceID].device->acquirePresentableImage(swapchain.impl());
      return swapchain.buffers()[index];
    }

    TextureRTV* DeviceGroupData::tryAcquirePresentableImage(Swapchain& swapchain)
    {
      int index = m_devices[SwapchainDeviceID].device->tryAcquirePresentableImage(swapchain.impl());
      if (index == -1)
        return nullptr;
      return &(swapchain.buffers()[index]);
    }

    Renderpass DeviceGroupData::createRenderpass()
    {
      auto handle = m_handles.allocate(ResourceType::Renderpass);
      for (auto&& device : m_devices)
      {
        device.device->createRenderpass(handle);
      }
      return Renderpass(sharedHandle(handle));
    }

    ComputePipeline DeviceGroupData::createComputePipeline(ComputePipelineDescriptor desc)
    {
      auto handle = m_handles.allocate(ResourceType::Pipeline);
      for(auto&& device : m_devices)
      {
        device.device->createPipeline(handle, desc);
      }
      return ComputePipeline(sharedHandle(handle), desc);
    }

    GraphicsPipeline DeviceGroupData::createGraphicsPipeline(GraphicsPipelineDescriptor desc)
    {
      auto handle = m_handles.allocate(ResourceType::Pipeline);
      for(auto&& device : m_devices)
      {
        device.device->createPipeline(handle, desc);
      }
      return GraphicsPipeline(sharedHandle(handle), desc);
    }

    std::shared_ptr<ResourceHandle> DeviceGroupData::sharedHandle(ResourceHandle handle)
    {
      return std::shared_ptr<ResourceHandle>(new ResourceHandle(handle),
      [weakDev = weak_from_this()](ResourceHandle* ptr)
      {
        if (auto dev = weakDev.lock())
        {
          dev->m_delayer.insert(0, *ptr);
        }
        delete ptr;
      });
    }

    Buffer DeviceGroupData::createBuffer(ResourceDescriptor desc)
    {
      desc = desc.setDimension(FormatDimension::Buffer);
      auto handle = m_handles.allocate(ResourceType::Buffer);
      // TODO: sharedbuffers
      for (auto& vdev : m_devices)
      {
        auto memRes = vdev.device->getReqs(desc); // memory requirements
        auto allo = vdev.heaps.allocate(memRes, [&](HeapDescriptor desc)
        {
          auto memHandle = m_handles.allocate(ResourceType::MemoryHeap);
          memHandle.setGpuId(vdev.id); 
          vdev.device->createHeap(memHandle, desc);
          return GpuHeap(memHandle, desc);
        }); // get heap corresponding to requirements
        vdev.device->createBuffer(handle, allo, desc); // assign and create buffer
        vdev.m_buffers[handle] = allo.allocation;
      }

      return Buffer(sharedHandle(handle), std::make_shared<ResourceDescriptor>(std::move(desc)));
    }

    Texture DeviceGroupData::createTexture(ResourceDescriptor desc)
    {
      auto handle = m_handles.allocate(ResourceType::Texture);
      // TODO: shared textures
      for (auto& vdev : m_devices)
      {
        auto memRes = vdev.device->getReqs(desc); // memory requirements
        auto allo = vdev.heaps.allocate(memRes, [&](HeapDescriptor desc)
        {
          auto memHandle = m_handles.allocate(ResourceType::MemoryHeap);
          memHandle.setGpuId(vdev.id); 
          vdev.device->createHeap(memHandle, desc);
          return GpuHeap(memHandle, desc);
        }); // get heap corresponding to requirements
        vdev.device->createTexture(handle, allo, desc); // assign and create buffer
        vdev.m_textures[handle] = allo.allocation;
      }

      return Texture(sharedHandle(handle), std::make_shared<ResourceDescriptor>(desc));
    }

    BufferSRV DeviceGroupData::createBufferSRV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto handle = m_handles.allocate(ResourceType::BufferSRV);
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      }
      return BufferSRV(buffer, sharedHandle(handle));
    }

    BufferUAV DeviceGroupData::createBufferUAV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto handle = m_handles.allocate(ResourceType::BufferUAV);
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      }
      return BufferUAV(buffer, sharedHandle(handle));
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

    TextureSRV DeviceGroupData::createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocate(ResourceType::TextureSRV);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      }
      return TextureSRV(texture, sharedHandle(handle));
    }

    TextureUAV DeviceGroupData::createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocate(ResourceType::TextureUAV);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      }
      return TextureUAV(texture, sharedHandle(handle));
    }

    TextureRTV DeviceGroupData::createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocate(ResourceType::TextureRTV);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::RenderTarget));
      }
      return TextureRTV(texture, sharedHandle(handle));
    }

    TextureDSV DeviceGroupData::createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocate(ResourceType::TextureDSV);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::DepthStencil));
      }
      return TextureDSV(texture, sharedHandle(handle));
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> , FormatType )
    {
      return DynamicBufferView();
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> , unsigned )
    {
      return DynamicBufferView();
    }

    bool DeviceGroupData::uploadInitialTexture(Texture& tex, CpuImage& image)
    {

      return true;
    }

    CommandGraph DeviceGroupData::startCommandGraph()
    {
      return CommandGraph(m_seqTracker.next()); 
    }

    void DeviceGroupData::submit(Swapchain& swapchain, CommandGraph graph)
    {
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
            //plist.list.emplace_back(std::move(nodes[i].list.list));
            plist.list.list.append(nodes[i].list.list);
            const auto & ref = nodes[i].needsResources();
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
            nativeList = m_devices[0].device->createDMAList();
            break;
          case CommandGraphNode::NodeType::Compute:
            nativeList = m_devices[0].device->createComputeList();
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            nativeList = m_devices[0].device->createGraphicsList();
          }

          nativeList->fillWith(m_devices[0].device, list.list.list);

          LiveCommandBuffer2 buffer{};
          buffer.deviceID = 0;
          buffer.started = InvalidSeqNum;
          buffer.lists.emplace_back(nativeList);

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
            optionalWaitSemaphore = m_devices[0].device->createSemaphore();
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
            buffer.fence = m_devices[0].device->createFence();
          }

          MemView<std::shared_ptr<FenceImpl>> viewToFences;

          if (buffer.fence)
          {
            viewToFences = buffer.fence;
          }

          switch (list.type)
          {
          case CommandGraphNode::NodeType::DMA:
            m_devices[0].device->submitDMA(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Compute:
            m_devices[0].device->submitCompute(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            break;
          case CommandGraphNode::NodeType::Graphics:
          default:
            m_devices[0].device->submitGraphics(buffer.lists, buffer.wait, buffer.signal, viewToFences);
          }
          m_buffers.emplace_back(buffer);
        }
      }
      m_buffers.back().started = graph.m_sequence;
      gc();
    }

    void DeviceGroupData::submit(CommandGraph graph)
    {
    }

    void DeviceGroupData::explicitSubmit(CommandGraph graph)
    {

    }

    void DeviceGroupData::garbageCollection()
    {
      auto handles = m_delayer.garbageCollection(m_seqTracker.completedTill());
      for (auto&& device : m_devices)
      {
        for (auto&& handle : handles)
        {
          if (handle.type == ResourceType::Buffer)
          {
            device.heaps.release(device.m_buffers[handle]);
          }
          if (handle.type == ResourceType::Texture)
          {
            device.heaps.release(device.m_textures[handle]);
          }
          auto ownerGpuId = handle.ownerGpuId();
          if (ownerGpuId == -1 || ownerGpuId == device.id)
          {
            device.device->releaseHandle(handle);
          }
        }
        auto removed = device.heaps.emptyHeaps();
        for (auto& it : removed)
        {
          device.device->releaseHandle(it.handle);
          m_handles.release(it.handle);
        }
      }
    }

    void DeviceGroupData::present(Swapchain & swapchain)
    {
      m_devices[SwapchainDeviceID].device->present(swapchain.impl(), swapchain.impl()->renderSemaphore());
    }
  }
}