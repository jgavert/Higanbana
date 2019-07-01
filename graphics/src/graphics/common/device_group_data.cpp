#include "graphics/common/device_group_data.hpp"
#include "graphics/common/gpudevice.hpp"
#include "graphics/common/graphicssurface.hpp"
#include "graphics/common/implementation.hpp"
#include "graphics/common/semaphore.hpp"
#include "graphics/common/cpuimage.hpp"
#include "graphics/common/swapchain.hpp"
#include "graphics/common/pipeline.hpp"
#include "graphics/common/commandgraph.hpp"

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
      for (auto& dev : m_devices)
      {
        auto checkQueue = [&](deque<LiveCommandBuffer2>& buffers)
        {
          if (!buffers.empty())
          {
            if (m_seqNumRequirements.size() > 3) // throttle so that we don't go too far ahead.
            {
              // force wait oldest
              int deviceID = buffers.front().deviceID;
              auto fence = buffers.front().fence;
              int i = 1;
              while (!fence && i < buffers.size())
              {
                if (buffers[i].fence)
                {
                  deviceID = buffers[i].deviceID;
                  fence = buffers[i].fence;
                  break;
                }
                ++i;
              }
              if (fence)
                dev.device->waitFence(fence);
            }
            int buffersToFree = 0;
            int buffersWithoutFence = 0;
            int bufferCount = static_cast<int>(buffers.size());
            for (int i = 0; i < bufferCount; ++i)
            {
              if (buffers[i].fence)
              {
                if (!dev.device->checkFence(buffers[i].fence))
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
              auto& buffer = buffers.front();
              if (buffer.started != -1 && buffer.started != currentSeqNum)
              {
                m_seqTracker.complete(buffer.started);
                currentSeqNum = buffer.started;
              }
              buffers.pop_front();
              buffersToFree--;
            }
          }
        };
        checkQueue(dev.m_gfxBuffers);
        checkQueue(dev.m_computeBuffers);
        checkQueue(dev.m_dmaBuffers);
      }
    }

    void DeviceGroupData::gc()
    {
      checkCompletedLists();
      garbageCollection();
    }

    DeviceGroupData::~DeviceGroupData()
    {
      waitGpuIdle();
      gc();
    }

    void DeviceGroupData::waitGpuIdle()
    {
      for (auto&& vdev : m_devices)
      {
        vdev.device->waitGpuIdle();
        auto waitBuffers = [&](deque<LiveCommandBuffer2>& buffer)
        {
          if (!buffer.empty())
          {
            for (auto&& liveBuffer : buffer)
            {
              if (liveBuffer.fence)
              {
                vdev.device->waitFence(liveBuffer.fence);
              }
            }
          }
        };
        waitBuffers(vdev.m_gfxBuffers);
        waitBuffers(vdev.m_computeBuffers);
        waitBuffers(vdev.m_dmaBuffers);
      }
      checkCompletedLists();
    }

    constexpr const int SwapchainDeviceID = 0;

    void setViewRange(const ShaderViewDescriptor& view, const ResourceDescriptor& desc, bool srv, ViewResourceHandle& handle)
    {
      auto mipOffset = static_cast<int>(view.m_mostDetailedMip);
      auto mipLevels = static_cast<int>(view.m_mipLevels);
      auto sliceOffset = static_cast<int>(view.m_arraySlice);
      auto arraySize = static_cast<int>(view.m_arraySize);

      if (mipLevels == -1)
      {
        mipLevels = static_cast<int>(desc.desc.miplevels - mipOffset);
      }

      if (arraySize == -1)
      {
        arraySize = static_cast<int>(desc.desc.arraySize - sliceOffset);
        if (desc.desc.dimension == FormatDimension::TextureCube)
        {
          arraySize = static_cast<int>((desc.desc.arraySize * 6) - sliceOffset);
        }
      }

      if (!srv)
      {
        mipLevels = 1;
      }
      handle.subresourceRange(desc.desc.miplevels, mipOffset, mipLevels, sliceOffset, arraySize);
    }

    void DeviceGroupData::configureBackbufferViews(Swapchain& sc)
    {
      vector<ResourceHandle> handles;
      // overcommitting with handles, 8 is max anyway...
      for (int i = 0; i < 8; i++)
      {
        auto handle = m_handles.allocateResource(ResourceType::Texture);
        handle.setGpuId(SwapchainDeviceID);
        handles.push_back(handle);
      }
      auto bufferCount = m_devices[SwapchainDeviceID].device->fetchSwapchainTextures(sc.impl(), handles);
      vector<TextureRTV> backbuffers;
      backbuffers.resize(bufferCount);
      for (int i = 0; i < bufferCount; ++i)
      {
        std::shared_ptr<ResourceHandle> handlePtr = std::shared_ptr<ResourceHandle>(new ResourceHandle(handles[i]), 
        [weakDevice = weak_from_this(), devId = SwapchainDeviceID](ResourceHandle* ptr)
        {
          if (auto dev = weakDevice.lock())
          {
            dev->m_handles.release(*ptr);
            dev->m_devices[devId].device->releaseHandle(*ptr);
          }
          delete ptr;
        });

        auto tex = Texture(handlePtr, std::make_shared<ResourceDescriptor>(sc.impl()->desc()));
        auto viewDesc = ShaderViewDescriptor();
        if (viewDesc.m_format == FormatType::Unknown)
        {
          viewDesc.m_format = tex.desc().desc.format;
        }

        auto subresources = tex.desc().desc.miplevels * tex.desc().desc.arraySize;
        m_devices[0].m_textureStates[*handlePtr].mips = tex.desc().desc.miplevels;
        m_devices[0].m_textureStates[*handlePtr].states = vector<ResourceState>(subresources, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Present, backend::TextureLayout::Undefined, QueueType::Unknown));

        auto handle = m_handles.allocateViewResource(ViewResourceType::TextureRTV, *handlePtr);
        m_devices[0].device->createTextureView(handle, tex.handle(), tex.desc(), viewDesc.setType(ResourceShaderType::RenderTarget));
        setViewRange(viewDesc, tex.desc(), false, handle);
        backbuffers[i] = TextureRTV(tex, sharedViewHandle(handle));
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

    std::optional<TextureRTV> DeviceGroupData::acquirePresentableImage(Swapchain& swapchain)
    {
      int index = m_devices[SwapchainDeviceID].device->acquirePresentableImage(swapchain.impl());
      if (index < 0 || index >= swapchain.buffers().size())
        return {};
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
      auto handle = m_handles.allocateResource(ResourceType::Renderpass);
      for (auto&& device : m_devices)
      {
        device.device->createRenderpass(handle);
      }
      return Renderpass(sharedHandle(handle));
    }

    ComputePipeline DeviceGroupData::createComputePipeline(ComputePipelineDescriptor desc)
    {
      auto handle = m_handles.allocateResource(ResourceType::Pipeline);
      for(auto&& device : m_devices)
      {
        device.device->createPipeline(handle, desc);
      }
      return ComputePipeline(sharedHandle(handle), desc);
    }

    GraphicsPipeline DeviceGroupData::createGraphicsPipeline(GraphicsPipelineDescriptor desc)
    {
      auto handle = m_handles.allocateResource(ResourceType::Pipeline);
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
          dev->m_delayer.insert(dev->m_currentSeqNum, *ptr);
        }
        delete ptr;
      });
    }

    std::shared_ptr<ViewResourceHandle> DeviceGroupData::sharedViewHandle(ViewResourceHandle handle)
    {
      return std::shared_ptr<ViewResourceHandle>(new ViewResourceHandle(handle),
      [weakDev = weak_from_this()](ViewResourceHandle* ptr)
      {
        if (auto dev = weakDev.lock())
        {
          dev->m_delayer.insert(dev->m_currentSeqNum, *ptr);
        }
        delete ptr;
      });
    }

    Buffer DeviceGroupData::createBuffer(ResourceDescriptor desc)
    {
      desc = desc.setDimension(FormatDimension::Buffer);
      auto handle = m_handles.allocateResource(ResourceType::Buffer);
      // TODO: sharedbuffers

      if (desc.desc.allowCrossAdapter) {
          F_ASSERT(desc.desc.hostGPU >= 0 && desc.desc.hostGPU < m_devices.size(), "Invalid hostgpu.");
          //handle.setGpuId(desc.desc.hostGPU);
          auto& vdev = m_devices[desc.desc.hostGPU];
          auto memRes = vdev.device->getReqs(desc); // memory requirements
          ResourceHandle heapHandle;
          auto allo = vdev.heaps.allocate(memRes, [&](HeapDescriptor desc)
          {
            heapHandle = m_handles.allocateResource(ResourceType::MemoryHeap);
            heapHandle.setGpuId(vdev.id); //??
            vdev.device->createHeap(heapHandle, desc);
            return GpuHeap(heapHandle, desc);
          }); // get heap corresponding to requirements
          vdev.device->createBuffer(handle, allo, desc); // assign and create buffer
          vdev.m_buffers[handle] = allo.allocation;
          vdev.m_bufferStates[handle] = ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::General, QueueType::Unknown);
          auto shared = vdev.device->openSharedHandle(allo);
          for (int i = 0; i < m_devices.size(); ++i)
          {
            if (i != desc.desc.hostGPU) // compatibility checks missing only allowed is same device dx12<->vk and different device dx12<->dx12
            {
              //auto memRes = m_devices[i].device->getReqs(desc); // memory requirements
              auto allo2 = m_devices[i].heaps.allocate(memRes, [&](HeapDescriptor desc)
              {
                auto hh = m_handles.allocateResource(ResourceType::MemoryHeap);
                hh.setGpuId(vdev.id); 
                m_devices[i].device->createHeapFromHandle(hh, shared);
                return GpuHeap(hh, desc);
              }); // get heap corresponding to requirements
              F_ASSERT(allo2.allocation.block.size == allo.allocation.block.size, "wtf!");
              F_ASSERT(allo2.allocation.block.offset == allo.allocation.block.offset, "wtf!");
              m_devices[i].device->createBuffer(handle, allo2, desc);
              m_devices[i].m_buffers[handle] = allo2.allocation;
              m_devices[i].m_bufferStates[handle] = ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::General, QueueType::Unknown);
            }
          }
      }
      else {
        for (auto& vdev : m_devices)
        {
          auto memRes = vdev.device->getReqs(desc); // memory requirements
          auto allo = vdev.heaps.allocate(memRes, [&](HeapDescriptor desc)
          {
            auto memHandle = m_handles.allocateResource(ResourceType::MemoryHeap);
            memHandle.setGpuId(vdev.id); 
            vdev.device->createHeap(memHandle, desc);
            return GpuHeap(memHandle, desc);
          }); // get heap corresponding to requirements
          vdev.device->createBuffer(handle, allo, desc); // assign and create buffer
          vdev.m_buffers[handle] = allo.allocation;
          vdev.m_bufferStates[handle] = ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::General, QueueType::Unknown);
        }
      }

      return Buffer(sharedHandle(handle), std::make_shared<ResourceDescriptor>(std::move(desc)));
    }

    Texture DeviceGroupData::createTexture(ResourceDescriptor desc)
    {
      auto handle = m_handles.allocateResource(ResourceType::Texture);

      if (desc.desc.allowCrossAdapter) // only interopt supported for now
      {
        F_ASSERT(desc.desc.hostGPU >= 0 && desc.desc.hostGPU < m_devices.size(), "Invalid hostgpu.");
        //handle.setGpuId(desc.desc.hostGPU);
        auto& vdev = m_devices[desc.desc.hostGPU];
        // texture
        vdev.device->createTexture(handle, desc);
        auto subresources = desc.desc.miplevels * desc.desc.arraySize;
        vdev.m_textureStates[handle].mips = desc.desc.miplevels;
        vdev.m_textureStates[handle].states = vector<ResourceState>(subresources, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::Undefined, QueueType::Unknown));
        // shared
        auto shared = vdev.device->openSharedHandle(handle);
        for (int i = 0; i < m_devices.size(); ++i)
        {
          if (i != desc.desc.hostGPU)
          {
            m_devices[i].device->createTextureFromHandle(handle, shared, desc);
            //m_devices[i].m_textures[handle] = allo.allocation;
            auto subresources = desc.desc.miplevels * desc.desc.arraySize;

            m_devices[i].m_textureStates[handle].mips = desc.desc.miplevels;
            m_devices[i].m_textureStates[handle].states = vector<ResourceState>(subresources, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::Undefined, QueueType::Unknown));
          }
        }
      }
      else
      {
        for (auto& vdev : m_devices)
        {
          auto memRes = vdev.device->getReqs(desc); // memory requirements
          auto allo = vdev.heaps.allocate(memRes, [&](HeapDescriptor desc)
          {
            auto memHandle = m_handles.allocateResource(ResourceType::MemoryHeap);
            memHandle.setGpuId(vdev.id); 
            vdev.device->createHeap(memHandle, desc);
            return GpuHeap(memHandle, desc);
          }); // get heap corresponding to requirements
          vdev.device->createTexture(handle, allo, desc); // assign and create buffer
          vdev.m_textures[handle] = allo.allocation;
          auto subresources = desc.desc.miplevels * desc.desc.arraySize;

          vdev.m_textureStates[handle].mips = desc.desc.miplevels;
          vdev.m_textureStates[handle].states = vector<ResourceState>(subresources, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::Undefined, QueueType::Unknown));
        }
      }

      return Texture(sharedHandle(handle), std::make_shared<ResourceDescriptor>(desc));
    }

    BufferSRV DeviceGroupData::createBufferSRV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto handle = m_handles.allocateViewResource(ViewResourceType::BufferSRV, buffer.handle());
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      }
      return BufferSRV(buffer, sharedViewHandle(handle));
    }

    BufferUAV DeviceGroupData::createBufferUAV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto handle = m_handles.allocateViewResource(ViewResourceType::BufferUAV, buffer.handle());
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      }
      return BufferUAV(buffer, sharedViewHandle(handle));
    }

    TextureSRV DeviceGroupData::createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocateViewResource(ViewResourceType::TextureSRV, texture.handle());
      setViewRange(viewDesc, texture.desc(), true, handle);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      }
      return TextureSRV(texture, sharedViewHandle(handle));
    }

    TextureUAV DeviceGroupData::createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocateViewResource(ViewResourceType::TextureUAV, texture.handle());
      setViewRange(viewDesc, texture.desc(), false, handle);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      }
      return TextureUAV(texture, sharedViewHandle(handle));
    }

    TextureRTV DeviceGroupData::createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocateViewResource(ViewResourceType::TextureRTV, texture.handle());
      setViewRange(viewDesc, texture.desc(), false, handle);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::RenderTarget));
      }
      return TextureRTV(texture, sharedViewHandle(handle));
    }

    TextureDSV DeviceGroupData::createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }

      auto handle = m_handles.allocateViewResource(ViewResourceType::TextureDSV, texture.handle());
      setViewRange(viewDesc, texture.desc(), false, handle);
      for (auto& vdev : m_devices)
      {
        vdev.device->createTextureView(handle, texture.handle(), texture.desc(), viewDesc.setType(ResourceShaderType::DepthStencil));
      }
      return TextureDSV(texture, sharedViewHandle(handle));
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> range, FormatType format)
    {
      auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
      m_delayer.insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->dynamic(handle, range, format);
      }
      return DynamicBufferView(handle);
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
      return CommandGraph(++m_currentSeqNum); 
    }

    void DeviceGroupData::fillCommandBuffer(std::shared_ptr<CommandBufferImpl> nativeList, VirtualDevice& vdev, CommandBuffer& buffer, QueueType queue)
    {
      BarrierSolver solver(vdev.m_bufferStates, vdev.m_textureStates);

      bool insideRenderpass = false;
      int drawIndexBeginRenderpass = 0;

      auto iter = buffer.begin();
      while((*iter)->type != PacketType::EndOfPackets)
      {
        CommandBuffer::PacketHeader* header = *iter;
        auto drawIndex = solver.addDrawCall();
        //F_ILOG("", "packet: %s", gfxpacket::packetTypeToString(header->type));
        switch (header->type)
        {
          case PacketType::RenderpassEnd:
          {
            insideRenderpass = false;
            break;
          }
          case PacketType::RenderpassBegin:
          {
            insideRenderpass = true;
            drawIndexBeginRenderpass = drawIndex;
            auto& packet = header->data<gfxpacket::RenderPassBegin>();
            for (auto&& rtv : packet.rtvs.convertToMemView())
            {
              solver.addTexture(drawIndex, rtv, ResourceState(backend::AccessUsage::ReadWrite, backend::AccessStage::Rendertarget, backend::TextureLayout::Rendertarget, queue));
            }
            if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
            {
              solver.addTexture(drawIndex, packet.dsv, ResourceState(backend::AccessUsage::ReadWrite, backend::AccessStage::DepthStencil, backend::TextureLayout::DepthStencil, queue));
            }
            break;
          }
          case PacketType::ResourceBinding:
          {
            auto& packet = header->data<gfxpacket::ResourceBinding>();
            auto stage = packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Graphics ? backend::AccessStage::Graphics : backend::AccessStage::Compute;
            stage = packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Raytracing ? backend::AccessStage::Raytrace : stage;

            auto usedDrawIndex = drawIndex;
            if (insideRenderpass)
            {
              usedDrawIndex = drawIndexBeginRenderpass;
            }
            for (auto&& resource : packet.resources.convertToMemView())
            {
              if (resource.type == ViewResourceType::BufferSRV)
              {
                solver.addBuffer(usedDrawIndex, resource, ResourceState(backend::AccessUsage::Read, stage, backend::TextureLayout::Undefined, queue));
              }
              else if (resource.type == ViewResourceType::BufferUAV)
              {
                solver.addBuffer(usedDrawIndex, resource, ResourceState(backend::AccessUsage::ReadWrite, stage, backend::TextureLayout::Undefined, queue));
              }
              else if (resource.type == ViewResourceType::TextureSRV)
              {
                solver.addTexture(usedDrawIndex, resource, ResourceState(backend::AccessUsage::Read, stage, backend::TextureLayout::ShaderReadOnly, queue));
              }
              else if (resource.type == ViewResourceType::TextureUAV)
              {
                solver.addTexture(usedDrawIndex, resource, ResourceState(backend::AccessUsage::ReadWrite, stage, backend::TextureLayout::General, queue));
              }
            }
            break;
          }
          case PacketType::BufferCopy:
          {
            auto& packet = header->data<gfxpacket::BufferCopy>();
            ViewResourceHandle src;
            src.resource = packet.src;
            ViewResourceHandle dst;
            dst.resource = packet.dst;
            solver.addBuffer(drawIndex, dst, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
            solver.addBuffer(drawIndex, src, ResourceState(backend::AccessUsage::Read,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
            break;
          }
          case PacketType::PrepareForPresent:
          {
            auto& packet = header->data<gfxpacket::PrepareForPresent>();
            ViewResourceHandle viewhandle{};
            viewhandle.resource = packet.texture;
            viewhandle.subresourceRange(1, 0, 1, 0, 1);
            solver.addTexture(drawIndex, viewhandle, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Present, backend::TextureLayout::Present, queue));
            break;
          }
          default:
            break;
        }

        iter++;
      }

      solver.makeAllBarriers();

      nativeList->fillWith(vdev.device, buffer, solver);
    }

#define IF_QUEUE_DEPENDENCY_DEBUG if constexpr (1)

    vector<FirstUseResource> DeviceGroupData::checkQueueDependencies(vector<PreparedCommandlist>& lists) {
      DynamicBitfield seenB;
      DynamicBitfield seenT;
      vector<FirstUseResource> fur;

      struct Intersection
      {
        DynamicBitfield buf;
        DynamicBitfield tex;

        bool hasSetBits() const
        {
          return buf.setBits() + tex.setBits();
        }
      };

      int graphicsList = -1;
      int computeList = -1;
      int dmaList = -1;
      int listIndex = 0;

      for (auto&& list : lists)
      {
        auto& queue = m_devices[list.device].qStates;
        auto checkDependency = [&](DynamicBitfield& buf, DynamicBitfield& tex)
        {
          Intersection intr;
          intr.buf = list.requirementsBuf.intersectFields(buf);
          intr.tex = list.requirementsTex.intersectFields(tex);
          buf.subtract(intr.buf);
          tex.subtract(intr.tex);
          return intr;
        };

        auto fSeen = list.requirementsBuf.exceptFields(seenB);
        fSeen.foreach([&](int id){
          IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "first seen Buffer %d", id);
          fur.emplace_back(FirstUseResource{list.device, ResourceType::Buffer, id, list.type});
        });
        seenB.add(fSeen);
        fSeen = list.requirementsTex.exceptFields(seenT);
        fSeen.foreach([&](int id){
          IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "first seen Texture %d", id);
          fur.emplace_back(FirstUseResource{list.device, ResourceType::Texture, id, list.type});
        });
        seenT.add(fSeen);

        if (list.type == QueueType::Graphics)
        {
          graphicsList = listIndex;
          IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "Graphics %d", listIndex);
          queue.gb.add(list.requirementsBuf);
          queue.gt.add(list.requirementsTex);
        }
        else if (list.type == QueueType::Compute)
        {
          computeList = listIndex;
          IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "Compute %d", listIndex);
          queue.cb.add(list.requirementsBuf);
          queue.ct.add(list.requirementsTex);
        }
        else
        {
          dmaList = listIndex;
          IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "dmalist %d", listIndex);
          queue.db.add(list.requirementsBuf);
          queue.dt.add(list.requirementsTex);
        }

        auto doAcquireReleasePairs = [&](int depList, const char* name, Intersection& ci, QueueType depType){
            if (depList >= 0)
              lists[depList].signal = true;
            IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "%d list had %s dependency!", listIndex, name);
            ci.buf.foreach([&](int id){
              IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "Buffer id: %d is transferred from list %d to %d", id, depList, listIndex);
              list.acquire.emplace_back(QueueTransfer{ResourceType::Buffer, id, depType});
              if (depList >= 0)
                lists[depList].release.emplace_back(QueueTransfer{ResourceType::Buffer, id, list.type});
            });
            ci.tex.foreach([&](int id){
              IF_QUEUE_DEPENDENCY_DEBUG F_ILOG("", "Texture id: %d is transferred from list %d to %d", id, depList, listIndex);
              list.acquire.emplace_back(QueueTransfer{ResourceType::Texture, id, depType});
              if (depList >= 0)
                lists[depList].release.emplace_back(QueueTransfer{ResourceType::Texture, id, list.type});
            });
        };

        if (list.type != QueueType::Graphics)
        {
          auto ci = checkDependency(queue.gb, queue.gt);
          if (ci.hasSetBits()) {
            list.waitGraphics = true;
            doAcquireReleasePairs(graphicsList, "Graphics", ci, QueueType::Graphics);
          }
        }
        if (list.type != QueueType::Compute)
        {
          auto ci = checkDependency(queue.cb, queue.ct);
          if (ci.hasSetBits()) {
            list.waitCompute = true;
            doAcquireReleasePairs(computeList, "Compute", ci, QueueType::Compute);
          }
        }
        if (list.type != QueueType::Dma)
        {
          auto ci = checkDependency(queue.db, queue.dt);
          if (ci.hasSetBits()) {
            list.waitDMA = true;
            doAcquireReleasePairs(dmaList, "DMA", ci, QueueType::Dma);
          }
        }
        listIndex++;
      }

      if (graphicsList >= 0 && !lists[graphicsList].isLastList)
      {
        lists[graphicsList].signal = true;
        lists[graphicsList].isLastList = true;
      }
      if (computeList >= 0 && !lists[computeList].isLastList)
      {
        lists[computeList].signal = true;
        lists[computeList].isLastList = true;
      }
      if(dmaList >= 0 && !lists[dmaList].isLastList)
      {
        lists[dmaList].signal = true;
        lists[dmaList].isLastList = true;
      }
      return fur;
    }

    const char* toString(QueueType type)
    {
      switch (type)
      {
        case QueueType::Graphics: return "GraphicsQueue";
        case QueueType::Compute: return "ComputeQueue";
        case QueueType::Dma: return "DMAQueue";
        case QueueType::External: return "ExternalQueue";
        case QueueType::Unknown:
        default: return "UnknownQueue";
      }
    }

    void DeviceGroupData::submit(Swapchain& swapchain, CommandGraph graph)
    {
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
          plist.list.list.append(nodes[i].list.list);
          plist.requirementsBuf = plist.requirementsBuf.unionFields(nodes[i].refBuf());
          plist.requirementsTex = plist.requirementsTex.unionFields(nodes[i].refTex());
          plist.acquireSema = nodes[i].acquireSemaphore;
          plist.presents = nodes[i].preparesPresent;

          i += 1;
          for (; i < static_cast<int>(nodes.size()); ++i)
          {
            if (nodes[i].type != plist.type)
              break;
            plist.list.list.append(nodes[i].list.list);
            plist.requirementsBuf = plist.requirementsBuf.unionFields(nodes[i].refBuf());
            plist.requirementsTex = plist.requirementsTex.unionFields(nodes[i].refTex());
            if (!plist.acquireSema)
            {
              plist.acquireSema = nodes[i].acquireSemaphore;
            }
            if (!plist.presents)
            {
              plist.presents = nodes[i].preparesPresent;
            }
          }

          lists.emplace_back(std::move(plist));
        }
        int lsize = lists.size();
        lists[lsize - 1].isLastList = true;

        auto firstUsageSeen = checkQueueDependencies(lists);

        // check states of first use resources
        for (auto&& resource : firstUsageSeen)
        {
          auto& dev = m_devices[resource.deviceID];
          if (resource.type == ResourceType::Buffer)
          {
            auto state = dev.m_bufferStates.at(resource.id);
            if (state.queue_index != QueueType::Unknown)
            {
              if (state.queue_index != resource.queue)
              {
                F_ILOG("", "Found buffer that needs handling, explicit queue transfer! %d %s -> %s", resource.id, toString(state.queue_index), toString(resource.queue));
              }
            }
          }
          else if (resource.type == ResourceType::Texture)
          {
            auto state = dev.m_textureStates.at(resource.id);
            auto firstStateIsEnough = state.states[0]; // because all subresource are required to be in right queue, just decided.
            if (firstStateIsEnough.queue_index != QueueType::Unknown)
            {
              if (firstStateIsEnough.queue_index != resource.queue)
              {
                F_ILOG("", "Found texture that needs handling, explicit queue transfer! %d %s -> %s", resource.id, toString(firstStateIsEnough.queue_index), toString(resource.queue));
              }
            }
          }
        }

        deque<LiveCommandBuffer2> readyLists;

        for (auto&& list : lists)
        {
          std::shared_ptr<CommandBufferImpl> nativeList;

          auto& vdev = m_devices[list.device];

          switch (list.type)
          {
          case QueueType::Dma:
            nativeList = vdev.device->createDMAList();
            break;
          case QueueType::Compute:
            nativeList = vdev.device->createComputeList();
            break;
          case QueueType::Graphics:
          default:
            nativeList = vdev.device->createGraphicsList();
          }


          LiveCommandBuffer2 buffer{};
          buffer.deviceID = list.device;
          buffer.started = m_seqTracker.next();
          buffer.lists.emplace_back(nativeList);
          buffer.cmdMemory = list.list.list;

          // barriers&commands
          fillCommandBuffer(nativeList, vdev, buffer.cmdMemory, list.type);

          readyLists.emplace_back(std::move(buffer));
        }

        for (auto&& list : lists)
        {
          LiveCommandBuffer2 buffer = std::move(readyLists.front());
          readyLists.pop_front();

          auto& vdev = m_devices[list.device];
          if (list.signal && list.type == QueueType::Graphics)
          {
            vdev.graphicsQSema = vdev.device->createSemaphore();
            buffer.signal.push_back(vdev.graphicsQSema);
          }
          if (list.signal && list.type == QueueType::Compute)
          {
            vdev.computeQSema = vdev.device->createSemaphore();
            buffer.signal.push_back(vdev.computeQSema);
          }
          if (list.signal && list.type == QueueType::Dma)
          {
            vdev.dmaQSema = vdev.device->createSemaphore();
            buffer.signal.push_back(vdev.dmaQSema);
          }

          if (list.waitGraphics)
            buffer.wait.push_back(vdev.graphicsQSema);
          if (list.waitCompute)
            buffer.wait.push_back(vdev.computeQSema);
          if (list.waitDMA)
            buffer.wait.push_back(vdev.dmaQSema);

          if (list.acquireSema)
          {
            buffer.wait.push_back(list.acquireSema);
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
            buffer.fence = vdev.device->createFence();
          }

          MemView<std::shared_ptr<FenceImpl>> viewToFences;

          if (buffer.fence)
          {
            viewToFences = buffer.fence;
          }

          switch (list.type)
          {
          case QueueType::Dma:
            vdev.device->submitDMA(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            vdev.device->submitCompute(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
            vdev.device->submitGraphics(buffer.lists, buffer.wait, buffer.signal, viewToFences);
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
          //m_buffers.emplace_back(buffer);
        }
      }
      //m_buffers.back().started = graph.m_sequence;
      m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
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
      auto completedListsTill = m_seqTracker.completedTill();
      while(!m_seqNumRequirements.empty() && m_seqNumRequirements.front() <= completedListsTill)
      {
        m_seqNumRequirements.pop_front();
        m_completedLists++;
      }
      auto garb = m_delayer.garbageCollection(m_completedLists);
      for (auto&& device : m_devices)
      {
        for (auto&& handle : garb.viewTrash)
        {
          auto ownerGpuId = handle.resource.ownerGpuId();
          if (handle.type == ViewResourceType::DynamicBufferSRV || ownerGpuId == -1 || ownerGpuId == device.id)
          {
            device.device->releaseViewHandle(handle);
          }
        }
        for (auto&& handle : garb.trash)
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
      for (auto&& handle : garb.trash)
      {
        m_handles.release(handle);
      }
      for (auto&& handle : garb.viewTrash)
      {
        m_handles.release(handle);
      }
    }

    void DeviceGroupData::present(Swapchain & swapchain)
    {
      m_devices[SwapchainDeviceID].device->present(swapchain.impl(), swapchain.impl()->renderSemaphore());
    }
  }
}