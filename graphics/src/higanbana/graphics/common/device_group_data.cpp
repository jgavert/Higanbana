#include "higanbana/graphics/common/device_group_data.hpp"
#include "higanbana/graphics/common/graphicssurface.hpp"
#include "higanbana/graphics/common/implementation.hpp"
#include "higanbana/graphics/common/semaphore.hpp"
#include "higanbana/graphics/common/cpuimage.hpp"
#include "higanbana/graphics/common/swapchain.hpp"
#include "higanbana/graphics/common/pipeline.hpp"
#include "higanbana/graphics/common/commandgraph.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"
#include "higanbana/graphics/common/shader_arguments_descriptor.hpp"
#include "higanbana/graphics/desc/shader_arguments_layout_descriptor.hpp"

#include <higanbana/core/math/utils.hpp>
#include <higanbana/core/profiling/profiling.hpp>

#include <execution>

namespace higanbana
{
  namespace backend
  {
    // device
    DeviceGroupData::DeviceGroupData(vector<std::shared_ptr<prototypes::DeviceImpl>> impls, vector<GpuInfo> infos)
    {
      m_delayer = std::make_unique<DelayedRelease>();
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
            if (m_seqNumRequirements.size() > 20) // throttle so that we don't go too far ahead.
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
              for (auto&& start : buffer.started)
              {
                if (start != -1 && start != currentSeqNum)
                {
                  m_seqTracker.complete(start);
                  currentSeqNum = start;
                }
              }
              if (!buffer.readbacks.empty())
              {
                for (auto&& rb : buffer.readbacks)
                {
                  auto handle = *rb.promiseId;
                  auto view = m_devices[buffer.deviceID].device->mapReadback(handle);
                  rb.promise->set_value(ReadbackData(rb.promiseId, view)); // TODO: SET READBACK HERE FOR USER
                }
              }
              auto queueId = buffer.submitID;
              auto foundTiming = false;
              for (auto&& submit : timeOnFlightSubmits)
              {
                if (submit.id == queueId)
                {
                  int offset = buffer.listIDs.front();
                  for (auto&& id : buffer.listIDs)
                  {
                    auto& timing = buffer.listTiming[id - offset];
                    auto& list = buffer.lists[id - offset];
                    timing.fromSubmitToFence.stop();
                    list->readbackTimestamps(m_devices[buffer.deviceID].device, timing.nodes);
                    timing.gpuTime.begin = timing.nodes.front().gpuTime.begin;
                    timing.gpuTime.end = timing.nodes.back().gpuTime.end;
                    submit.lists.push_back(timing);
                    foundTiming = true;
                  }
                  break;
                }
              }
              HIGAN_ASSERT(foundTiming, "Err, didn't find timing, error");
              HIGAN_ASSERT(timeOnFlightSubmits.size() < 20, "wtf!");
              //HIGAN_LOGi("problem %zu?\n",timeOnFlightSubmits.size());
              while(!timeOnFlightSubmits.empty())
              {
                auto& first = timeOnFlightSubmits.front();
                if (first.listsCount == first.lists.size())
                {
                  timeSubmitsFinished.push_back(first);
                  timeOnFlightSubmits.pop_front();
                  continue;
                }
                else
                {
                  break;
                }
              }
              while (timeSubmitsFinished.size() > 10)
              {
                timeSubmitsFinished.pop_front();
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
      auto _gc = HIGAN_CPU_BRACKET("DeviceGroupData::GarbageCollection");
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
          dev->m_delayer->insert(dev->m_currentSeqNum, *ptr);
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
          dev->m_delayer->insert(dev->m_currentSeqNum, *ptr);
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
          HIGAN_ASSERT(desc.desc.hostGPU >= 0 && desc.desc.hostGPU < m_devices.size(), "Invalid hostgpu.");
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
              HIGAN_ASSERT(allo2.allocation.block.size == allo.allocation.block.size, "wtf!");
              HIGAN_ASSERT(allo2.allocation.block.offset == allo.allocation.block.offset, "wtf!");
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
        HIGAN_ASSERT(desc.desc.hostGPU >= 0 && desc.desc.hostGPU < m_devices.size(), "Invalid hostgpu.");
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

    BufferIBV DeviceGroupData::createBufferIBV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto handle = m_handles.allocateViewResource(ViewResourceType::BufferIBV, buffer.handle());
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::IndexBuffer));
      }
      return BufferIBV(buffer, sharedViewHandle(handle));
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
      m_delayer->insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->dynamic(handle, range, format);
      }
      return DynamicBufferView(handle, formatSizeInfo(format).pixelSize, range.size_bytes());
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> range, unsigned stride)
    {
      auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
      m_delayer->insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->dynamic(handle, range, stride);
      }
      return DynamicBufferView(handle, stride, range.size_bytes());
    }

    DynamicBufferView DeviceGroupData::dynamicImage(MemView<uint8_t> range, unsigned rowPitch)
    {
      auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
      m_delayer->insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->dynamicImage(handle, range, rowPitch);
      }
      return DynamicBufferView(handle, rowPitch, range.size_bytes());
    }

    ShaderArgumentsLayout DeviceGroupData::createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor desc)
    {
      auto handle = m_handles.allocateResource(ResourceType::ShaderArgumentsLayout);
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->createShaderArgumentsLayout(handle, desc);
      }
      return ShaderArgumentsLayout(sharedHandle(handle), desc.structDeclarations(), desc.getResources());
    }

    ShaderArguments DeviceGroupData::createShaderArguments(ShaderArgumentsDescriptor& binding)
    {
      auto handle = m_handles.allocateResource(ResourceType::ShaderArguments);
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->createShaderArguments(handle, binding);
        vdev.shaderArguments[handle] = binding.bResources();
      }
      return ShaderArguments(sharedHandle(handle), binding.bResources());
    }

    bool DeviceGroupData::uploadInitialTexture(Texture& tex, CpuImage& image)
    {
      auto graph = CommandGraph(InvalidSeqNum, m_commandBuffers);

      vector<DynamicBufferView> allBufferToImages;
      auto arraySize = image.desc().desc.arraySize;
      for (auto slice = 0u; slice < arraySize; ++slice)
      {
        for (auto mip = 0u; mip < image.desc().desc.miplevels; ++mip)
        {
          auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
          m_delayer->insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
          auto sr = image.subresource(mip, slice);
          for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
          {
            vdev.device->dynamicImage(handle, makeByteView(sr.data(), sr.size()), static_cast<unsigned>(sr.rowPitch()));
          }
          allBufferToImages.emplace_back(DynamicBufferView(handle, formatSizeInfo(image.desc().desc.format).pixelSize, sr.slicePitch()));
        }
      }
      for (auto& vdev : m_devices)
      {
        for (auto slice = 0u; slice < arraySize; ++slice)
        {
          auto node = graph.createPass(std::string("copyTexture") + std::to_string(slice), QueueType::Graphics, vdev.id);
          for (auto mip = 0u; mip < image.desc().desc.miplevels; ++mip)
          {
            auto index = slice * image.desc().desc.miplevels + mip;
            node.copy(tex, allBufferToImages[index], Subresource().mip(mip).slice(slice));
          }
          graph.addPass(std::move(node));
        }
      }
      submit(std::optional<Swapchain>(), graph, ThreadedSubmission::ParallelUnsequenced);
      return true;
    }

    CommandGraph DeviceGroupData::startCommandGraph()
    {
      return CommandGraph(++m_currentSeqNum, m_commandBuffers); 
    }

    // we have given promises of readbacks to user, we need backing memory for those readbacks and patch commandbuffer with valid resources.
    void DeviceGroupData::generateReadbackCommands(VirtualDevice& vdev, MemView<backend::CommandBuffer>& buffers, QueueType queue, vector<ReadbackPromise>& readbacks)
    {
      auto _gr = HIGAN_CPU_BRACKET("generateReadbackCommands");
      int currentReadback = 0;
      for (auto&& buffer : buffers)
      {
        auto iter = buffer.begin();
        while((*iter)->type != PacketType::EndOfPackets)
        {
          CommandBuffer::PacketHeader* header = *iter;
          switch(header->type)
          {
            case PacketType::ReadbackBuffer:
            {
              auto handle = m_handles.allocateResource(ResourceType::ReadbackBuffer);
              handle.setGpuId(vdev.id);
              readbacks[currentReadback].promiseId = sharedHandle(handle);
              auto& packet = header->data<gfxpacket::ReadbackBuffer>();
              vdev.device->readbackBuffer(handle, packet.numBytes);
              packet.dst = handle;
              currentReadback++;
              break;
            }
            default:
            {
              break;
            }
          }
          iter++;
        }
      }
    }

    void DeviceGroupData::fillCommandBuffer(
      std::shared_ptr<CommandBufferImpl> nativeList,
      VirtualDevice& vdev,
      MemView<CommandBuffer>& buffers,
      QueueType queue,
      vector<QueueTransfer>& acquires,
      vector<QueueTransfer>& releases,
      CommandListTiming& timing)
    {
      timing.barrierAdd.start();
      BarrierSolver solver(vdev.m_bufferStates, vdev.m_textureStates);

      bool insideRenderpass = false;
      int drawIndexBeginRenderpass = 0;

      gfxpacket::ResourceBinding::BindingType currentBoundType = gfxpacket::ResourceBinding::BindingType::Graphics;
      ResourceHandle boundSets[4] = {};

      for (auto&& buffer : buffers)
      {
        auto iter = buffer.begin();
        while((*iter)->type != PacketType::EndOfPackets)
        {
          CommandBuffer::PacketHeader* header = *iter;
          auto drawIndex = solver.addDrawCall();
          HIGAN_ASSERT(drawIndex >= 0, "ups, infinite");
          if (drawIndex == 0)
          {
            for (auto&& acq : acquires)
            {
              ResourceHandle h;
              h.id = acq.id;
              h.type = acq.type;
              ViewResourceHandle view;
              view.resource = h;
              view.subresourceRange(1, 0, 1, 0, 1);
              if (h.type == ResourceType::Buffer)
              {
                solver.addBuffer(drawIndex, view, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::Undefined, acq.fromOrTo));
              }
              else
              {
                solver.addTexture(drawIndex, view, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::Undefined, acq.fromOrTo));
              }
            }
          }
          //HIGAN_LOGi( "packet: %s", gfxpacket::packetTypeToString(header->type));
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
                solver.addTexture(drawIndex, rtv, ResourceState(backend::AccessUsage::ReadWrite, backend::AccessStage::Rendertarget, backend::TextureLayout::Rendertarget,  queue));
              }
              if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
              {
                solver.addTexture(drawIndex, packet.dsv, ResourceState(backend::AccessUsage::ReadWrite, backend::AccessStage::DepthStencil, backend::TextureLayout::DepthStencil,  queue));
              }
              break;
            }
            case PacketType::ResourceBinding:
            {
              auto& packet = header->data<gfxpacket::ResourceBinding>();
              auto stage = packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Graphics ? backend::AccessStage::Graphics : backend::AccessStage::Compute;
              stage = packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Raytracing ? backend::AccessStage::Raytrace : stage;
              if (packet.graphicsBinding != currentBoundType)
              {
                boundSets[0] = {};
                boundSets[1] = {};
                boundSets[2] = {};
                boundSets[3] = {};
                currentBoundType = packet.graphicsBinding;
              }

              auto usedDrawIndex = drawIndex;
              if (insideRenderpass)
              {
                usedDrawIndex = drawIndexBeginRenderpass;
              }
              int i = 0;
              for (auto&& handle : packet.resources.convertToMemView())
              {
                if (boundSets[i] != handle)
                {
                  boundSets[i] = handle;
                  const auto& views = vdev.shaderArguments[handle];
                  for (auto&& resource : views)
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
                }
                ++i;
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
            case PacketType::DynamicBufferCopy:
            {
              auto& packet = header->data<gfxpacket::DynamicBufferCopy>();
              ViewResourceHandle dst;
              dst.resource = packet.dst;
              solver.addBuffer(drawIndex, dst, ResourceState(backend::AccessUsage::Write,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              break;
            }
            case PacketType::ReadbackBuffer:
            {
              auto& packet = header->data<gfxpacket::ReadbackBuffer>();
              ViewResourceHandle src;
              src.resource = packet.src;
              solver.addBuffer(drawIndex, src, ResourceState(backend::AccessUsage::Read,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              break;
            }
            case PacketType::UpdateTexture:
            {
              auto& packet = header->data<gfxpacket::UpdateTexture>();
              ViewResourceHandle viewhandle{};
              viewhandle.resource = packet.tex;
              viewhandle.subresourceRange(packet.allMips, packet.mip, 1, packet.slice, 1);
              solver.addTexture(drawIndex, viewhandle, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Transfer, backend::TextureLayout::TransferDst, queue));
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
      }
      if (!releases.empty())
      {
        buffers[buffers.size()-1].insert<gfxpacket::ReleaseFromQueue>();
        auto drawIndex = solver.addDrawCall();
        for (auto&& rel : releases)
        {
          ResourceHandle h;
          h.id = rel.id;
          h.type = rel.type;
          ViewResourceHandle view;
          view.resource = h;
          view.subresourceRange(1, 0, 1, 0, 1);
          if (h.type == ResourceType::Buffer)
          {
            solver.addBuffer(drawIndex, view, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Common, backend::TextureLayout::Undefined, rel.fromOrTo));
          }
          else
          {
            solver.addTexture(drawIndex, view, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Common, backend::TextureLayout::Undefined, rel.fromOrTo));
          }
        }
      }

      timing.barrierAdd.stop();

      timing.barrierSolveLocal.start();
      solver.localBarrierPass1();
      timing.barrierSolveLocal.stop();

      timing.barrierSolveGlobal.start();
      solver.globalBarrierPass2();
      timing.barrierSolveGlobal.stop();

      timing.fillNativeList.start();
      nativeList->fillWith(vdev.device, buffers, solver);
      timing.fillNativeList.stop();
    }

    void DeviceGroupData::firstPassBarrierSolve(
      VirtualDevice& vdev,
      MemView<CommandBuffer>& buffers,
      QueueType queue,
      vector<QueueTransfer>& acquires,
      vector<QueueTransfer>& releases,
      CommandListTiming& timing,
      BarrierSolver& solver)
    {
      timing.barrierAdd.start();

      bool insideRenderpass = false;
      int drawIndexBeginRenderpass = 0;

      gfxpacket::ResourceBinding::BindingType currentBoundType = gfxpacket::ResourceBinding::BindingType::Graphics;
      ResourceHandle boundSets[4] = {};

      for (auto&& buffer : buffers)
      {
        auto iter = buffer.begin();
        while((*iter)->type != PacketType::EndOfPackets)
        {
          CommandBuffer::PacketHeader* header = *iter;
          auto drawIndex = solver.addDrawCall();
          HIGAN_ASSERT(drawIndex >= 0, "ups, infinite");
          if (drawIndex == 0)
          {
            for (auto&& acq : acquires)
            {
              ResourceHandle h;
              h.id = acq.id;
              h.type = acq.type;
              ViewResourceHandle view;
              view.resource = h;
              view.subresourceRange(1, 0, 1, 0, 1);
              if (h.type == ResourceType::Buffer)
              {
                solver.addBuffer(drawIndex, view, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::Undefined, acq.fromOrTo));
              }
              else
              {
                solver.addTexture(drawIndex, view, ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::Undefined, acq.fromOrTo));
              }
            }
          }
          //HIGAN_LOGi( "packet: %s", gfxpacket::packetTypeToString(header->type));
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
                solver.addTexture(drawIndex, rtv, ResourceState(backend::AccessUsage::ReadWrite, backend::AccessStage::Rendertarget, backend::TextureLayout::Rendertarget,  queue));
              }
              if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
              {
                solver.addTexture(drawIndex, packet.dsv, ResourceState(backend::AccessUsage::ReadWrite, backend::AccessStage::DepthStencil, backend::TextureLayout::DepthStencil,  queue));
              }
              break;
            }
            case PacketType::ResourceBinding:
            {
              auto& packet = header->data<gfxpacket::ResourceBinding>();
              auto stage = packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Graphics ? backend::AccessStage::Graphics : backend::AccessStage::Compute;
              stage = packet.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Raytracing ? backend::AccessStage::Raytrace : stage;
              if (packet.graphicsBinding != currentBoundType)
              {
                boundSets[0] = {};
                boundSets[1] = {};
                boundSets[2] = {};
                boundSets[3] = {};
                currentBoundType = packet.graphicsBinding;
              }

              auto usedDrawIndex = drawIndex;
              if (insideRenderpass)
              {
                usedDrawIndex = drawIndexBeginRenderpass;
              }
              int i = 0;
              for (auto&& handle : packet.resources.convertToMemView())
              {
                if (boundSets[i] != handle)
                {
                  boundSets[i] = handle;
                  const auto& views = vdev.shaderArguments[handle];
                  for (auto&& resource : views)
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
                }
                ++i;
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
            case PacketType::DynamicBufferCopy:
            {
              auto& packet = header->data<gfxpacket::DynamicBufferCopy>();
              ViewResourceHandle dst;
              dst.resource = packet.dst;
              solver.addBuffer(drawIndex, dst, ResourceState(backend::AccessUsage::Write,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              break;
            }
            case PacketType::ReadbackBuffer:
            {
              auto& packet = header->data<gfxpacket::ReadbackBuffer>();
              ViewResourceHandle src;
              src.resource = packet.src;
              solver.addBuffer(drawIndex, src, ResourceState(backend::AccessUsage::Read,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              break;
            }
            case PacketType::UpdateTexture:
            {
              auto& packet = header->data<gfxpacket::UpdateTexture>();
              ViewResourceHandle viewhandle{};
              viewhandle.resource = packet.tex;
              viewhandle.subresourceRange(packet.allMips, packet.mip, 1, packet.slice, 1);
              solver.addTexture(drawIndex, viewhandle, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Transfer, backend::TextureLayout::TransferDst, queue));
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
      }
      if (!releases.empty())
      {
        buffers[buffers.size()-1].insert<gfxpacket::ReleaseFromQueue>();
        auto drawIndex = solver.addDrawCall();
        for (auto&& rel : releases)
        {
          ResourceHandle h;
          h.id = rel.id;
          h.type = rel.type;
          ViewResourceHandle view;
          view.resource = h;
          view.subresourceRange(1, 0, 1, 0, 1);
          if (h.type == ResourceType::Buffer)
          {
            solver.addBuffer(drawIndex, view, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Common, backend::TextureLayout::Undefined, rel.fromOrTo));
          }
          else
          {
            solver.addTexture(drawIndex, view, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Common, backend::TextureLayout::Undefined, rel.fromOrTo));
          }
        }
      }

      timing.barrierAdd.stop();

      timing.barrierSolveLocal.start();
      solver.localBarrierPass1();
      timing.barrierSolveLocal.stop();
    }

    void DeviceGroupData::globalPassBarrierSolve(CommandListTiming& timing, BarrierSolver& solver)
    {
      timing.barrierSolveGlobal.start();
      solver.globalBarrierPass2();
      timing.barrierSolveGlobal.stop();
    }

    void DeviceGroupData::fillNativeList(std::shared_ptr<CommandBufferImpl>& nativeList, VirtualDevice& vdev, MemView<CommandBuffer>& buffers, BarrierSolver& solver, CommandListTiming& timing)
    {
      timing.fillNativeList.start();
      nativeList->fillWith(vdev.device, buffers, solver);
      timing.fillNativeList.stop();
    }

#define IF_QUEUE_DEPENDENCY_DEBUG if constexpr (0)

    vector<FirstUseResource> DeviceGroupData::checkQueueDependencies(vector<PreparedCommandlist>& lists) {
      IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "\nRenderGraph %d\n", m_currentSeqNum);
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
          IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "first seen Buffer %d\n", id);
          fur.emplace_back(FirstUseResource{list.device, ResourceType::Buffer, id, list.type});
        });
        seenB.add(fSeen);
        fSeen = list.requirementsTex.exceptFields(seenT);
        fSeen.foreach([&](int id){
          IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "first seen Texture %d\n", id);
          fur.emplace_back(FirstUseResource{list.device, ResourceType::Texture, id, list.type});
        });
        seenT.add(fSeen);

        if (list.type == QueueType::Graphics)
        {
          graphicsList = listIndex;
          IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "Graphics %d\n", listIndex);
          queue.gb.add(list.requirementsBuf);
          queue.gt.add(list.requirementsTex);
        }
        else if (list.type == QueueType::Compute)
        {
          computeList = listIndex;
          IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "Compute %d\n", listIndex);
          queue.cb.add(list.requirementsBuf);
          queue.ct.add(list.requirementsTex);
        }
        else
        {
          dmaList = listIndex;
          IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "dmalist %d\n", listIndex);
          queue.db.add(list.requirementsBuf);
          queue.dt.add(list.requirementsTex);
        }

        auto doAcquireReleasePairs = [&](int depList, const char* name, Intersection& ci, QueueType depType){
            if (depList >= 0)
              lists[depList].signal = true;
            IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "%d list had %s dependency!\n", listIndex, name);
            ci.buf.foreach([&](int id){
              IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "Buffer id: %d is transferred from list %d to %d\n", id, depList, listIndex);
              list.acquire.emplace_back(QueueTransfer{ResourceType::Buffer, id, depType});
              if (depList >= 0)
                lists[depList].release.emplace_back(QueueTransfer{ResourceType::Buffer, id, list.type});
            });
            ci.tex.foreach([&](int id){
              IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "Texture id: %d is transferred from list %d to %d\n", id, depList, listIndex);
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

    // submit function breakdown
    vector<PreparedCommandlist> DeviceGroupData::prepareNodes(vector<CommandGraphNode>& nodes)
    {
        vector<PreparedCommandlist> lists;
        int i = 0;
        while (i < static_cast<int>(nodes.size()))
        {
          auto* firstList = &nodes[i];

          PreparedCommandlist plist{};
          plist.type = firstList->type;
          auto list = std::move(nodes[i].list->list);
          plist.buffers.emplace_back(std::move(list));
          //HIGAN_LOGi("%d node %zu bytes\n", i, plist.buffers.back().sizeBytes());
          plist.requirementsBuf = plist.requirementsBuf.unionFields(nodes[i].refBuf());
          plist.requirementsTex = plist.requirementsTex.unionFields(nodes[i].refTex());
          plist.readbacks = nodes[i].m_readbackPromises;
          plist.acquireSema = nodes[i].acquireSemaphore;
          plist.presents = nodes[i].preparesPresent;
          plist.timing.nodes.emplace_back(nodes[i].timing);
          plist.timing.type = firstList->type;

          i += 1;
          for (; i < static_cast<int>(nodes.size()); ++i)
          {
            if (nodes[i].type != plist.type)
              break;
            if (nodes[i].list->list.sizeBytes() > 500)
              break;
            plist.buffers.emplace_back(std::move(nodes[i].list->list));
            //HIGAN_LOGi("%d node %zu bytes\n", i, plist.buffers.back().sizeBytes());
            plist.requirementsBuf = plist.requirementsBuf.unionFields(nodes[i].refBuf());
            plist.requirementsTex = plist.requirementsTex.unionFields(nodes[i].refTex());
            plist.readbacks.insert(plist.readbacks.end(), nodes[i].m_readbackPromises.begin(), nodes[i].m_readbackPromises.end());
            plist.timing.nodes.emplace_back(nodes[i].timing);
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
        return lists;
    }

    void DeviceGroupData::returnResouresToOriginalQueues(vector<PreparedCommandlist>& lists, vector<backend::FirstUseResource>& firstUsageSeen)
    {
      for (auto&& resource : firstUsageSeen)
      {
        for (int index = static_cast<int>(lists.size()) - 1; index >= 0; --index )
        {
          bool found = false;
          auto queType = lists[index].type;
          for (auto&& acquire : lists[index].acquire)
          {
            if (acquire.type == resource.type && acquire.id == resource.id && queType != resource.queue)
            {
              IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "Found last acquire that needs releasing, explicit queue transfer! %d %s -> %s\n", resource.id, toString(queType), toString(resource.queue));
              auto release = acquire;
              release.fromOrTo = resource.queue;
              lists[index].release.push_back(release);
              found = true;
              break;
            }
          }
          if (found)
            break;
        }
      }
    }

    void DeviceGroupData::handleQueueTransfersWithinRendergraph(vector<PreparedCommandlist>& lists, vector<backend::FirstUseResource>& firstUsageSeen)
    {
      vector<QueueTransfer> uhOhGfx;
      vector<QueueTransfer> uhOhCompute;
      vector<QueueTransfer> uhOhDma;

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
              IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "Found buffer that needs handling, explicit queue transfer! %d %s -> %s\n", resource.id, toString(state.queue_index), toString(resource.queue));
              QueueTransfer info{};
              info.fromOrTo = resource.queue;
              info.id = resource.id;
              info.type = resource.type;
              if (state.queue_index == QueueType::Graphics)
                uhOhGfx.push_back(info);
              if (state.queue_index == QueueType::Compute)
                uhOhCompute.push_back(info);
              if (state.queue_index == QueueType::Dma)
                uhOhDma.push_back(info);
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
              IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "Found texture that needs handling, explicit queue transfer! %d %s -> %s\n", resource.id, toString(firstStateIsEnough.queue_index), toString(resource.queue));
              QueueTransfer info{};
              info.fromOrTo = resource.queue;
              info.id = resource.id;
              info.type = resource.type;
              if (firstStateIsEnough.queue_index == QueueType::Graphics)
                uhOhGfx.push_back(info);
              if (firstStateIsEnough.queue_index == QueueType::Compute)
                uhOhCompute.push_back(info);
              if (firstStateIsEnough.queue_index == QueueType::Dma)
                uhOhDma.push_back(info);
            }
          }
        }
      }

      HIGAN_ASSERT(uhOhGfx.empty(), "Need to make lists to fix these resources...");
      HIGAN_ASSERT(uhOhCompute.empty(), "Need to make lists to fix these resources...");
      HIGAN_ASSERT(uhOhDma.empty(), "Need to make lists to fix these resources...");
    }

    deque<LiveCommandBuffer2> DeviceGroupData::makeLiveCommandBuffers(vector<PreparedCommandlist>& lists, uint64_t submitID)
    {
      auto _makeBuffers = HIGAN_CPU_BRACKET("makeLiveCommandBuffers");
      deque<LiveCommandBuffer2> readyLists;
      int listID = 0;
      
      LiveCommandBuffer2 buffer{};
      buffer.queue = lists[0].type;

      for (auto&& list : lists)
      {
        auto _list = HIGAN_CPU_BRACKET("list in lists");
        if (list.type != buffer.queue)
        {
          auto _readyListsAdd = HIGAN_CPU_BRACKET("readyLists.emplace_back");
          readyLists.emplace_back(std::move(buffer));
          buffer = LiveCommandBuffer2{};
        }
        std::shared_ptr<CommandBufferImpl> nativeList;
        buffer.submitID = submitID;
        buffer.listTiming.push_back(list.timing);
        buffer.queue = list.type;
        buffer.listTiming.back().cpuBackendTime.start();
        buffer.fence = nullptr;

        auto& vdev = m_devices[list.device];

        {
          auto _natList = HIGAN_CPU_BRACKET("createNativeList");
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
        }

        buffer.deviceID = list.device;
        buffer.started.push_back(m_seqTracker.next());
        buffer.lists.emplace_back(nativeList);

        auto buffersView = makeMemView(list.buffers.data(), list.buffers.size());
        buffer.listIDs.push_back(listID);
        // patch readbacks
        generateReadbackCommands(vdev, buffersView, list.type, list.readbacks);
        {
          auto _addReadbacks = HIGAN_CPU_BRACKET("addReadbacks");
          buffer.readbacks.insert(buffer.readbacks.end(), list.readbacks.begin(), list.readbacks.end());
        }
        //buffer.readbacks = std::move(list.readbacks);

        listID++;
      }
      auto _finalEmplace = HIGAN_CPU_BRACKET("readyLists.emplace_back");
      readyLists.emplace_back(std::move(buffer));
      return readyLists;
    }

    void DeviceGroupData::submit(std::optional<Swapchain> swapchain, CommandGraph& graph, ThreadedSubmission multithreaded)
    {
      auto funcProfile = HIGAN_CPU_BRACKET("DeviceGroupData::submit");
      SubmitTiming timing = graph.m_timing;
      timing.id = m_submitIDs++;
      timing.listsCount = 0;
      timing.timeBeforeSubmit.stop();
      timing.submitCpuTime.start();
      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        vector<PreparedCommandlist> lists;
        {
          auto _nodeAdd = HIGAN_CPU_BRACKET("addNodes");
          timing.addNodes.start();
          lists = prepareNodes(nodes);
          timing.addNodes.stop();
        }

        {
          auto _graphSolve = HIGAN_CPU_BRACKET("GraphSolve");
          timing.graphSolve.start();
          auto firstUsageSeen = checkQueueDependencies(lists);
          returnResouresToOriginalQueues(lists, firstUsageSeen);
          handleQueueTransfersWithinRendergraph(lists, firstUsageSeen);
          timing.graphSolve.stop();
        }


        timing.fillCommandLists.start();

        auto readyLists = makeLiveCommandBuffers(lists, timing.id);
        timing.listsCount = lists.size();

        vector<BarrierSolver> solvers;
        //solvers.resize(readyLists.size());

        {
          auto _barriers = HIGAN_CPU_BRACKET("create BarrierSolvers...");
          for (auto&& list : readyLists)
          {
            auto _list = HIGAN_CPU_BRACKET("list");
            auto& vdev = m_devices[list.deviceID];
            for (auto&& id : list.listIDs)
            {
              auto _list = HIGAN_CPU_BRACKET("id in listIds");
              solvers.push_back(BarrierSolver(vdev.m_bufferStates, vdev.m_textureStates));
              //solvers[list.listID] = BarrierSolver(vdev.m_bufferStates, vdev.m_textureStates);
            }
          }
        }

        std::for_each(std::execution::par_unseq, std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list) {
          int offset = list.listIDs[0];
          auto _OuterLoopFirstPass = HIGAN_CPU_BRACKET("OuterLoopFirstPass");
          std::for_each(std::execution::par_unseq, std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            auto _innerLoopFirstPass = HIGAN_CPU_BRACKET("InnerLoopFirstPass");
            auto& solver = solvers[id];
            auto& buffer = lists[id];
            auto& vdev = m_devices[list.deviceID];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, list.listTiming[id - offset], solver);
          });
        });
        
        {
          auto _globalPass = HIGAN_CPU_BRACKET("globalPass");
          for (auto&& list : readyLists)
          {
            int offset = list.listIDs[0];
            for (auto&& id : list.listIDs)
            {
              auto& solver = solvers[id];
              // this is order dependant
              globalPassBarrierSolve(list.listTiming[id - offset], solver);
              // also parallel
            }
          }
        }

        std::for_each(std::execution::par_unseq, std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list)
        {
          int offset = list.listIDs[0];
          auto _OuterLoopFirstPass = HIGAN_CPU_BRACKET("OuterLoopFillNativeList");
          std::for_each(std::execution::par_unseq, std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            auto _innerLoopFirstPass = HIGAN_CPU_BRACKET("InnerLoopFillNativeList");
            auto& buffer = lists[id];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            auto& vdev = m_devices[list.deviceID];
            auto& solver = solvers[id];
            fillNativeList(list.lists[id-offset], vdev, buffersView, solver, list.listTiming[id - offset]);
            list.listTiming[id - offset].cpuBackendTime.stop();
          });
        });

        timing.fillCommandLists.stop();

        timing.submitSolve.start();
        // submit can be "multithreaded" also in the order everything finished, but not in current shape where readyLists is modified.
        //for (auto&& list : lists)
        auto _submitLists = HIGAN_CPU_BRACKET("Submit Lists");
        while(!readyLists.empty())
        {
          LiveCommandBuffer2 buffer = std::move(readyLists.front());
          readyLists.pop_front();

          auto& vdev = m_devices[buffer.deviceID];
          std::optional<std::shared_ptr<FenceImpl>> viewToFence;
          for (auto&& id : buffer.listIDs)
          {
            auto& list = lists[id];
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

            if (list.waitGraphics && vdev.graphicsQSema)
              buffer.wait.push_back(vdev.graphicsQSema);
            if (list.waitCompute && vdev.computeQSema)
              buffer.wait.push_back(vdev.computeQSema);
            if (list.waitDMA && vdev.dmaQSema)
              buffer.wait.push_back(vdev.dmaQSema);

            if (list.acquireSema)
            {
              buffer.wait.push_back(list.acquireSema);
            }

            if (list.presents && swapchain)
            {
              auto sc = swapchain.value();
              auto presenene = sc.impl()->renderSemaphore();
              if (presenene)
              {
                buffer.signal.push_back(presenene);
              }
            }

            if (list.isLastList || !buffer.readbacks.empty())
            {
              buffer.fence = vdev.device->createFence();
            }

            if (buffer.fence)
            {
              viewToFence = buffer.fence;
            }

            for (auto&& timing : buffer.listTiming)
            {
              timing.fromSubmitToFence.start();
            }
          }
          switch (buffer.queue)
          {
          case QueueType::Dma:
            vdev.device->submitDMA(buffer.lists, buffer.wait, buffer.signal, viewToFence);
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            vdev.device->submitCompute(buffer.lists, buffer.wait, buffer.signal, viewToFence);
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
            vdev.device->submitGraphics(buffer.lists, buffer.wait, buffer.signal, viewToFence);
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
          for (auto&& id : buffer.listIDs)
          {
            auto& list = lists[id];
            for (auto&& buffer : list.buffers)
            {
              m_commandBuffers.free(std::move(buffer));
            }
          }
        }
        timing.submitSolve.stop();
      }
      timing.submitCpuTime.stop();
      timeOnFlightSubmits.push_back(timing);
      if (graph.m_sequence != InvalidSeqNum)
      {
        m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
        gc();
      }
    }

    void DeviceGroupData::garbageCollection()
    {
      auto completedListsTill = m_seqTracker.completedTill();
      while(!m_seqNumRequirements.empty() && m_seqNumRequirements.front() <= completedListsTill)
      {
        m_seqNumRequirements.pop_front();
        m_completedLists++;
      }
      auto garb = m_delayer->garbageCollection(m_completedLists);
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
          if (handle.type == ResourceType::ReadbackBuffer && ownerGpuId == device.id)
          {
            device.device->unmapReadback(handle);
            device.device->releaseHandle(handle);
          }
          else if (ownerGpuId == -1 || ownerGpuId == device.id)
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