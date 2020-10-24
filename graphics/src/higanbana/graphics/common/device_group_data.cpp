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
#include <algorithm>
#include <future>
#include <optional>

#define IF_QUEUE_DEPENDENCY_DEBUG if constexpr (0)

namespace higanbana
{
  namespace backend
  {
    // device
    DeviceGroupData::DeviceGroupData(vector<std::shared_ptr<prototypes::DeviceImpl>> impls, vector<GpuInfo> infos) {
      HIGAN_CPU_FUNCTION_SCOPE();
      m_delayer = std::make_unique<DelayedRelease>();
      // all devices have their own heap managers
      for (int i = 0; i < impls.size(); ++i)
      {
        auto t1 = impls[i]->createTimelineSemaphore();
        auto t2 = impls[i]->createTimelineSemaphore();
        auto t3 = impls[i]->createTimelineSemaphore();
        m_devices.push_back({i, impls[i], HeapManager(), infos[i], t1, t2, t3, 0, 0, 0});
      }
      // so all gpu's should have a reference to other gpu's shared timeline.
      if (m_devices.size() > 1) {
        if (m_devices.size() == 2 && infos[1].api == GraphicsApi::Vulkan) {
          // interopt
          // create vulkan timeline semaphore and share it with DX12
          auto timeline = m_devices[0].device->createSharedSemaphore();
          m_devices[0].sharedTimelines.resize(2, timeline);
          auto shared = m_devices[0].device->openSharedHandle(timeline);
          auto sharedTimeline = m_devices[1].device->createSemaphoreFromHandle(shared);
          m_devices[1].sharedTimelines.resize(2, sharedTimeline);
        }
        else {
          HIGAN_ASSERT(false, "unimplemented.");
          /*
          auto sharedTimeline = m_devices[0].device->createTimelineSemaphore();
          for (int i = 1; i < m_devices.size(); i++) {
            //m_devices[i].device->openSharedHandle
          }*/
        }
      }
    }

    void DeviceGroupData::checkCompletedLists() {
      HIGAN_CPU_FUNCTION_SCOPE();
      for (auto& dev : m_devices)
      {
        auto gfxQueueReached = dev.device->completedValue(dev.timelineGfx);
        auto cptQueueReached = dev.device->completedValue(dev.timelineCompute);
        auto dmaQueueReached = dev.device->completedValue(dev.timelineDma);

        auto checkQueue = [&](deque<LiveCommandBuffer2>& buffers)
        {
          if (!buffers.empty())
          {
            if (m_seqNumRequirements.size() > 20) // throttle so that we don't go too far ahead.
            {
              // force wait oldest list
              HIGAN_CPU_BRACKET("Wait for gpu...");
              switch(buffers.front().queue) {
                case QueueType::Dma:
                  dev.device->waitTimeline(dev.timelineDma, buffers.front().dmaValue);
                  break;
                case QueueType::Compute:
                  dev.device->waitTimeline(dev.timelineCompute, buffers.front().cptValue);
                  break;
                case QueueType::Graphics:
                default:
                  dev.device->waitTimeline(dev.timelineGfx, buffers.front().gfxValue);
              }
            }
            int buffersToFree = 0;
            int buffersWithoutFence = 0;
            int bufferCount = static_cast<int>(buffers.size());
            for (int i = 0; i < bufferCount; ++i)
            {
              auto& buffer = buffers[i];
              if (buffer.gfxValue > gfxQueueReached
                || buffer.cptValue > cptQueueReached
                || buffer.dmaValue > dmaQueueReached) {
                break;
              }
              else
              {
                if (buffer.fence && !dev.device->checkFence(buffer.fence))
                {
                  HIGAN_CPU_BRACKET("Wait for gpu...");
                  dev.device->waitFence(buffer.fence);
                  //HIGAN_ASSERT(false, "failed, we thought everything was ready... %d > %d, %d > %d, %d > %d", buffer.gfxValue, gfxQueueReached, buffer.cptValue, cptQueueReached, buffer.dmaValue, dmaQueueReached);
                  break;
                }
                buffersToFree++;
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
                for (auto&& rbs : buffer.readbacks)
                {
                  for (auto&& rb : rbs)
                  {
                    auto handle = *rb.promiseId;
                    auto view = m_devices[buffer.deviceID].device->mapReadback(handle);
                    rb.promise->set_value(ReadbackData(rb.promiseId, view)); // TODO: SET READBACK HERE FOR USER
                  }
                }
              }
              auto queueId = buffer.submitID;
              auto foundTiming = false;
              vector<GraphNodeTiming> dmaTimings;
              dmaTimings.push_back(GraphNodeTiming{});
              {
                HIGAN_CPU_BRACKET("Collect gpu timestamps for profiling");
                for (auto&& submit : timeOnFlightSubmits)
                {
                  if (submit.id == queueId)
                  {
                    // dma list first
                    if (buffer.dmaListConstants) {
                      if (buffer.dmaListConstants->readbackTimestamps(m_devices[buffer.deviceID].device, dmaTimings)) {
                        //HIGAN_LOGi("Had constants!\n");
                        auto& timing = dmaTimings.back();
                        std::string copy = std::to_string(submit.id) + ": Copy Constants ";
                        HIGAN_GPU_BRACKET_FULL(buffer.deviceID, QueueType::Dma, copy.c_str(), timing.gpuTime.begin, timing.gpuTime.nanoseconds());
                        buffer.listTiming.front().constantsDmaTime.begin = timing.gpuTime.begin;
                        buffer.listTiming.front().constantsDmaTime.end = timing.gpuTime.end;
                      }
                    }

                    int offset = buffer.listIDs.front();
                    for (auto&& id : buffer.listIDs)
                    {
                      auto& timing = buffer.listTiming[id - offset];
                      auto& list = buffer.lists[id - offset];
                      timing.fromSubmitToFence.stop();
                      if (list->readbackTimestamps(m_devices[buffer.deviceID].device, timing.nodes)) {
                        timing.gpuTime.begin = timing.nodes.front().gpuTime.begin;
                        timing.gpuTime.end = timing.nodes.back().gpuTime.end;
                        std::string cmdListName = std::to_string(submit.id) + ": Commandlist " + std::to_string(id - offset);
                        HIGAN_GPU_BRACKET_FULL(buffer.deviceID, buffer.queue, cmdListName.c_str(), timing.gpuTime.begin, timing.gpuTime.nanoseconds());
                        for (auto&& block : timing.nodes)
                        {
                          HIGAN_GPU_BRACKET_FULL(buffer.deviceID, buffer.queue, block.nodeName, block.gpuTime.begin, block.gpuTime.nanoseconds());
                        }
                      }
                      submit.lists.push_back(timing);
                      foundTiming = true;
                    }
                    break;
                  }
                }
              }
              //HIGAN_ASSERT(foundTiming, "Err, didn't find timing, error");
              //HIGAN_ASSERT(timeOnFlightSubmits.size() < 40, "wtf!");
              //HIGAN_LOGi("problem %zu?\n",timeOnFlightSubmits.size());
              while(!timeOnFlightSubmits.empty())
              {
                HIGAN_CPU_BRACKET("move submit timings...");
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

              {
                HIGAN_CPU_BRACKET("pop buffer :D");
                buffers.pop_front();
              }
              buffersToFree--;
            }
          }
        };
        {
          HIGAN_CPU_BRACKET("check gfx buffers");
          checkQueue(dev.m_gfxBuffers);
        }
        {
          HIGAN_CPU_BRACKET("check gfx buffers");
          checkQueue(dev.m_computeBuffers);
        }
        {
          HIGAN_CPU_BRACKET("check gfx buffers");
          checkQueue(dev.m_dmaBuffers);
        }
      }
    }

    void extractShaderDebug(MemView<uint32_t> rb) {
      auto offset = 1;
      auto type = rb[offset];
      while(type != 0 && offset < HIGANBANA_SHADER_DEBUG_WIDTH - 8)
      {
        switch (type)
        {
          // uint types
          case 1:
          {
            HIGAN_SLOG("Shader Debug", "uint  %u\n", rb[offset+1]);
            break;
          }
          case 2:
          {
            HIGAN_SLOG("Shader Debug", "uint2 %u %u\n", rb[offset+1], rb[offset+2]);
            break;
          }
          case 3:
          {
            HIGAN_SLOG("Shader Debug", "uint3 %u %u %u\n", rb[offset+1], rb[offset+2], rb[offset+3]);
            break;
          }
          case 4:
          {
            HIGAN_SLOG("Shader Debug", "uint4 %u %u %u %u\n", rb[offset+1], rb[offset+2], rb[offset+3], rb[offset+4]);
            break;
          }
          // int types
          case 5:
          {
            int val;
            memcpy(&val, &rb[offset+1], sizeof(int));
            HIGAN_SLOG("Shader Debug", "int  %d\n", val);
            break;
          }
          case 6:
          {
            int val[2];
            memcpy(&val, &rb[offset+1], sizeof(int)*2);
            HIGAN_SLOG("Shader Debug", "int2 %d %d\n", val[0], val[1]);
            break;
          }
          case 7:
          {
            int val[3];
            memcpy(&val, &rb[offset+1], sizeof(int)*3);
            HIGAN_SLOG("Shader Debug", "int3 %d %d %d\n", val[0], val[1], val[2]);
            break;
          }
          case 8:
          {
            int val[4];
            memcpy(&val, &rb[offset+1], sizeof(int)*4);
            HIGAN_SLOG("Shader Debug", "int4 %d %d %d %d\n", val[0], val[1], val[2], val[3]);
            break;
          }
          // float types
          case 9:
          {
            float val;
            memcpy(&val, &rb[offset+1], sizeof(float));
            HIGAN_SLOG("Shader Debug", "float  %f\n", val);
            break;
          }
          case 10:
          {
            float val[2];
            memcpy(&val, &rb[offset+1], sizeof(float)*2);
            HIGAN_SLOG("Shader Debug", "float2 %f %f\n", val[0], val[1]);
            break;
          }
          case 11:
          {
            float val[3];
            memcpy(&val, &rb[offset+1], sizeof(float)*3);
            HIGAN_SLOG("Shader Debug", "float3 %f %f %f\n", val[0], val[1], val[2]);
            break;
          }
          case 12:
          {
            float val[4];
            memcpy(&val, &rb[offset+1], sizeof(float)*4);
            HIGAN_SLOG("Shader Debug", "float4 %f %f %f %f\n", val[0], val[1], val[2], val[3]);
            break;
          }
          default:
          //HIGAN_ASSERT(false, "wut, what type %d", type);
            HIGAN_LOGi("unknown type %d\n", type);
            return;
        }
        offset += (type-1)%4+2;
        type = rb[offset];
      }
    }

    void DeviceGroupData::gc() {
      HIGAN_CPU_BRACKET("DeviceGroupData::GarbageCollection");
      checkCompletedLists();
      garbageCollection();
      while(!m_shaderDebugReadbacks.empty() && m_shaderDebugReadbacks.front().ready())
      {
        auto data = m_shaderDebugReadbacks.front().get();
        auto view = data.view<uint32_t>();
        if (view[0] != 0)
        {
          extractShaderDebug(view);
        }
        m_shaderDebugReadbacks.pop_front();
      }
    }

    DeviceGroupData::~DeviceGroupData() {
      HIGAN_CPU_FUNCTION_SCOPE();
      waitGpuIdle();
      gc();
    }

    void DeviceGroupData::waitGpuIdle() {
      HIGAN_CPU_FUNCTION_SCOPE();
      for (auto&& vdev : m_devices)
      {
        vdev.device->waitGpuIdle();
      }
      gc();
    }

    constexpr const int SwapchainDeviceID = 0;

    void setViewRange(const ShaderViewDescriptor& view, const ResourceDescriptor& desc, bool srv, ViewResourceHandle& handle) {
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

    void DeviceGroupData::configureBackbufferViews(Swapchain& sc) {
      HIGAN_CPU_FUNCTION_SCOPE();
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

    Swapchain DeviceGroupData::createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor) {
      HIGAN_CPU_FUNCTION_SCOPE();
      // assume that first device is the one in charge of presenting. TODO: swapchain is created always on device 0
      auto sc = m_devices[SwapchainDeviceID].device->createSwapchain(surface, descriptor);
      auto swapchain = Swapchain(sc);
      // get backbuffers to swapchain
      configureBackbufferViews(swapchain);
      return swapchain;
    }

    void DeviceGroupData::adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor) {
      HIGAN_CPU_FUNCTION_SCOPE();
      std::lock_guard<std::mutex> guard(m_presentMutex);
      while (!m_asyns.empty())
      {
        m_asyns.back().wait();
        m_asyns.pop_back();
      }
      // stop gpu, possibly wait for last 'present' by inserting only fence to queue.
      auto fenceForSwapchain = m_devices[SwapchainDeviceID].device->createFence();
      // assuming that only graphics queue accesses swapchain resources.
      {
        HIGAN_CPU_BRACKET("submit fence");
        m_devices[SwapchainDeviceID].device->submitGraphics({}, {}, {}, {}, {}, fenceForSwapchain);
      }
      {
        HIGAN_CPU_BRACKET("wait the fence");
        m_devices[SwapchainDeviceID].device->waitFence(fenceForSwapchain);
      }
      {
        HIGAN_CPU_BRACKET("waitGpuIdle");
        waitGpuIdle();
      }
      // wait all idle work.
      // release current swapchain backbuffers
      {
        HIGAN_CPU_BRACKET("releaseSwapchainBackbuffers");
        swapchain.setBackbuffers({}); // technically this frees the textures if they are not used anywhere.
      }
      // go collect the trash.
      gc();

      // blim
      m_devices[SwapchainDeviceID].device->adjustSwapchain(swapchain.impl(), descriptor);
      // get new backbuffers... seems like we do it here.

      {
        HIGAN_CPU_BRACKET("configureBackbufferViews");
        configureBackbufferViews(swapchain);
      }
    }

    std::optional<std::pair<int, TextureRTV>> DeviceGroupData::acquirePresentableImage(Swapchain& swapchain) {
      HIGAN_CPU_BRACKET("acquirePresentableImage");
      std::lock_guard<std::mutex> guard(m_presentMutex);
      while (!m_asyns.empty())
      {
        m_asyns.back().wait();
        m_asyns.pop_back();
      }
      int index = m_devices[SwapchainDeviceID].device->acquirePresentableImage(swapchain.impl());
      if (index < 0 || index >= swapchain.buffers().size())
        return {};
      return std::make_pair(index, swapchain.buffers()[index]);
    }

    TextureRTV* DeviceGroupData::tryAcquirePresentableImage(Swapchain& swapchain) {
      HIGAN_CPU_BRACKET("try acquirePresentableImage");
      std::lock_guard<std::mutex> guard(m_presentMutex);
      while (!m_asyns.empty())
      {
        m_asyns.back().wait();
        m_asyns.pop_back();
      }
      int index = m_devices[SwapchainDeviceID].device->tryAcquirePresentableImage(swapchain.impl());
      if (index == -1)
        return nullptr;
      return &(swapchain.buffers()[index]);
    }

    Renderpass DeviceGroupData::createRenderpass() {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateResource(ResourceType::Renderpass);
      for (auto&& device : m_devices)
      {
        device.device->createRenderpass(handle);
      }
      return Renderpass(sharedHandle(handle));
    }

    ComputePipeline DeviceGroupData::createComputePipeline(ComputePipelineDescriptor desc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateResource(ResourceType::Pipeline);
      for(auto&& device : m_devices)
      {
        device.device->createPipeline(handle, desc);
      }
      return ComputePipeline(sharedHandle(handle), desc);
    }

    GraphicsPipeline DeviceGroupData::createGraphicsPipeline(GraphicsPipelineDescriptor desc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateResource(ResourceType::Pipeline);
      for(auto&& device : m_devices)
      {
        device.device->createPipeline(handle, desc);
      }
      return GraphicsPipeline(sharedHandle(handle), desc);
    }

    std::shared_ptr<ResourceHandle> DeviceGroupData::sharedHandle(ResourceHandle handle) {
      HIGAN_CPU_FUNCTION_SCOPE();
      return std::shared_ptr<ResourceHandle>(new ResourceHandle(handle),
      [weakDev = weak_from_this()](ResourceHandle* ptr)
      {
        if (auto dev = weakDev.lock())
        {
          dev->m_delayer->insert(dev->m_currentSeqNum+1, *ptr);
        }
        delete ptr;
      });
    }

    std::shared_ptr<ViewResourceHandle> DeviceGroupData::sharedViewHandle(ViewResourceHandle handle) {
      HIGAN_CPU_FUNCTION_SCOPE();
      return std::shared_ptr<ViewResourceHandle>(new ViewResourceHandle(handle),
      [weakDev = weak_from_this()](ViewResourceHandle* ptr)
      {
        if (auto dev = weakDev.lock())
        {
          dev->m_delayer->insert(dev->m_currentSeqNum+1, *ptr);
        }
        delete ptr;
      });
    }

    void DeviceGroupData::validateResourceDescriptor(ResourceDescriptor& desc) {
      auto crossAdapterPossible = m_devices.size() > 1;
      if (!crossAdapterPossible) desc.desc.allowCrossAdapter = false;
    }

    Buffer DeviceGroupData::createBuffer(ResourceDescriptor desc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      desc = desc.setDimension(FormatDimension::Buffer);
      auto handle = m_handles.allocateResource(ResourceType::Buffer);

      if (desc.desc.allowCrossAdapter) {
        handle.sharedResource = 1;
        handle.setGpuId(desc.desc.hostGPU);
        HIGAN_ASSERT(desc.desc.hostGPU >= 0 && desc.desc.hostGPU < m_devices.size(), "Invalid hostgpu.");
        auto& vdev = m_devices[desc.desc.hostGPU];
        auto memRes = vdev.device->getReqs(desc); // memory requirements
        ResourceHandle heapHandle;
        auto allo = vdev.heaps.allocate(memRes, [&](HeapDescriptor desc)
        {
          heapHandle = m_handles.allocateResource(ResourceType::MemoryHeap);
          heapHandle.setGpuId(vdev.id); //??
          heapHandle.sharedResource = 1;
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

    Texture DeviceGroupData::createTexture(ResourceDescriptor desc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateResource(ResourceType::Texture);

      if (desc.desc.allowCrossAdapter) // only interopt supported for now
      {
        handle.sharedResource = 1;
        handle.setGpuId(desc.desc.hostGPU);
        HIGAN_ASSERT(desc.desc.hostGPU >= 0 && desc.desc.hostGPU < m_devices.size(), "Invalid hostgpu.");
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

    BufferIBV DeviceGroupData::createBufferIBV(Buffer buffer, ShaderViewDescriptor viewDesc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateViewResource(ViewResourceType::BufferIBV, buffer.handle());
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::IndexBuffer));
      }
      return BufferIBV(buffer, sharedViewHandle(handle), std::make_shared<ShaderViewDescriptor>(viewDesc));
    }

    BufferSRV DeviceGroupData::createBufferSRV(Buffer buffer, ShaderViewDescriptor viewDesc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateViewResource(ViewResourceType::BufferSRV, buffer.handle());
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      }
      return BufferSRV(buffer, sharedViewHandle(handle), std::make_shared<ShaderViewDescriptor>(viewDesc));
    }

    BufferUAV DeviceGroupData::createBufferUAV(Buffer buffer, ShaderViewDescriptor viewDesc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateViewResource(ViewResourceType::BufferUAV, buffer.handle());
      for (auto& vdev : m_devices)
      {
        vdev.device->createBufferView(handle, buffer.handle(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      }
      return BufferUAV(buffer, sharedViewHandle(handle), std::make_shared<ShaderViewDescriptor>(viewDesc));
    }

    TextureSRV DeviceGroupData::createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc) {
      HIGAN_CPU_FUNCTION_SCOPE();
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

    TextureUAV DeviceGroupData::createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc) {
      HIGAN_CPU_FUNCTION_SCOPE();
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

    TextureRTV DeviceGroupData::createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc) {
      HIGAN_CPU_FUNCTION_SCOPE();
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

    TextureDSV DeviceGroupData::createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc) {
      HIGAN_CPU_FUNCTION_SCOPE();
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

    size_t DeviceGroupData::availableDynamicMemory(int gpu) {
      HIGAN_ASSERT(gpu >= 0 && gpu < m_devices.size(), "please no");
      return m_devices[gpu].device->availableDynamicMemory();
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> range, FormatType format) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
      m_delayer->insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->dynamic(handle, range, format);
      }
      return DynamicBufferView(handle, formatSizeInfo(format).pixelSize, range.size_bytes());
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> range, unsigned stride) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
      m_delayer->insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->dynamic(handle, range, stride);
      }
      return DynamicBufferView(handle, stride, range.size_bytes());
    }

    DynamicBufferView DeviceGroupData::dynamicImage(MemView<uint8_t> range, unsigned rowPitch) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
      m_delayer->insert(m_currentSeqNum, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->dynamicImage(handle, range, rowPitch);
      }
      return DynamicBufferView(handle, rowPitch, range.size_bytes());
    }

    ShaderArgumentsLayout DeviceGroupData::createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor desc) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateResource(ResourceType::ShaderArgumentsLayout);
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->createShaderArgumentsLayout(handle, desc);
      }
      return ShaderArgumentsLayout(sharedHandle(handle), desc.structDeclarations(), desc.getResources(), desc.bindless);
    }

    ShaderArguments DeviceGroupData::createShaderArguments(ShaderArgumentsDescriptor& binding) {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto handle = m_handles.allocateResource(ResourceType::ShaderArguments);
      for (auto& vdev : m_devices) // uh oh :D TODO: maybe not dynamic buffers for all gpus? close eyes for now
      {
        vdev.device->createShaderArguments(handle, binding);
        vdev.shaderArguments[handle] = binding.bResources();
      }
      return ShaderArguments(sharedHandle(handle), binding.bResources());
    }

    bool DeviceGroupData::uploadInitialTexture(Texture& tex, CpuImage& image) {
      HIGAN_CPU_FUNCTION_SCOPE();

      auto allBytes = 0;
      auto arraySize = image.desc().desc.arraySize;
      for (auto slice = 0u; slice < arraySize; ++slice) {
        for (auto mip = 0u; mip < image.desc().desc.miplevels; ++mip) {
          allBytes += image.subresource(mip, slice).size() + 1024*16;
        }
      }
      auto deviceMemory = m_devices[0].device->availableDynamicMemory();
      if (deviceMemory < allBytes) {
        waitGpuIdle(); 
      }
      deviceMemory = m_devices[0].device->availableDynamicMemory();
      HIGAN_ASSERT(deviceMemory > allBytes, "Don't have enough memory to begin with.");

      auto graph = startCommandGraph();

      vector<DynamicBufferView> allBufferToImages;
      for (auto slice = 0u; slice < arraySize; ++slice)
      {
        for (auto mip = 0u; mip < image.desc().desc.miplevels; ++mip)
        {
          auto handle = m_handles.allocateViewResource(ViewResourceType::DynamicBufferSRV, ResourceHandle());
          m_delayer->insert(m_currentSeqNum+1, handle); // dynamic buffers will be released immediately with delay. "one frame/use"
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
      // todo: don't know...
      //submit(std::optional<Swapchain>(), graph, ThreadedSubmission::ParallelUnsequenced);
      submitST(std::optional<Swapchain>(), graph);
      return true;
    }

    CommandGraph DeviceGroupData::startCommandGraph() {
      return CommandGraph(++m_currentSeqNum, m_commandBuffers); 
    }

    void DeviceGroupData::firstPassBarrierSolve(
      VirtualDevice& vdev,
      MemView<CommandBuffer*>& buffers,
      QueueType queue,
      vector<QueueTransfer>& acquires,
      vector<QueueTransfer>& releases,
      CommandListTiming& timing,
      BarrierSolver& solver,
      vector<ReadbackPromise>& readbacks,
      bool isFirstList) {
      HIGAN_CPU_FUNCTION_SCOPE();
      timing.barrierAdd.start();

      bool insideRenderpass = false;
      int drawIndexBeginRenderpass = 0;
      int currentReadback = 0;

      gfxpacket::ResourceBinding::BindingType currentBoundType = gfxpacket::ResourceBinding::BindingType::Graphics;
      ResourceHandle boundSets[HIGANBANA_USABLE_SHADER_ARGUMENT_SETS] = {};

      for (auto&& buffer : buffers)
      {
        auto iter = buffer->begin();
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
              view.resource = h.rawValue;
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
              src.resource = packet.src.rawValue;
              ViewResourceHandle dst;
              dst.resource = packet.dst.rawValue;
              solver.addBuffer(drawIndex, dst, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              solver.addBuffer(drawIndex, src, ResourceState(backend::AccessUsage::Read,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              break;
            }
            case PacketType::DynamicBufferCopy:
            {
              auto& packet = header->data<gfxpacket::DynamicBufferCopy>();
              ViewResourceHandle dst;
              dst.resource = packet.dst.rawValue;
              solver.addBuffer(drawIndex, dst, ResourceState(backend::AccessUsage::Write,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              break;
            }
            case PacketType::ReadbackBuffer:
            {
              auto& packet = header->data<gfxpacket::ReadbackBuffer>();
              ViewResourceHandle src;
              src.resource = packet.src.rawValue;
              solver.addBuffer(drawIndex, src, ResourceState(backend::AccessUsage::Read,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              {
                // READBACKS HANDLED HERE, sneakily put here.
                auto handle = m_handles.allocateResource(ResourceType::ReadbackBuffer);
                handle.setGpuId(vdev.id);
                readbacks[currentReadback].promiseId = sharedHandle(handle);
                vdev.device->readbackBuffer(handle, packet.numBytes);
                packet.dst = handle;
                currentReadback++;
              }
              break;
            }
            case PacketType::UpdateTexture:
            {
              auto& packet = header->data<gfxpacket::UpdateTexture>();
              ViewResourceHandle viewhandle{};
              viewhandle.resource = packet.tex.rawValue;
              viewhandle.subresourceRange(packet.allMips, packet.mip, 1, packet.slice, 1);
              solver.addTexture(drawIndex, viewhandle, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Transfer, backend::TextureLayout::TransferDst, queue));
              break;
            }
            case PacketType::TextureToBufferCopy:
            {
              auto& packet = header->data<gfxpacket::TextureToBufferCopy>();
              ViewResourceHandle dst;
              dst.resource = packet.dstBuffer.rawValue;
              solver.addBuffer(drawIndex, dst, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              ViewResourceHandle src;
              src.resource = packet.srcTexture.rawValue;
              src.subresourceRange(packet.allMips, packet.mip, 1, packet.slice, 1);
              solver.addTexture(drawIndex, src, ResourceState(backend::AccessUsage::Read,  backend::AccessStage::Transfer, backend::TextureLayout::TransferSrc, queue));
              break;
            }
            case PacketType::BufferToTextureCopy:
            {
              auto& packet = header->data<gfxpacket::BufferToTextureCopy>();
              ViewResourceHandle dst;
              dst.resource = packet.dstTexture.rawValue;
              dst.subresourceRange(packet.allMips, packet.mip, 1, packet.slice, 1);
              solver.addTexture(drawIndex, dst, ResourceState(backend::AccessUsage::Write, backend::AccessStage::Transfer, backend::TextureLayout::TransferDst, queue));
              ViewResourceHandle src;
              src.resource = packet.srcBuffer.rawValue;
              solver.addBuffer(drawIndex, src, ResourceState(backend::AccessUsage::Read,  backend::AccessStage::Transfer, backend::TextureLayout::Undefined, queue));
              break;
            }
            case PacketType::PrepareForPresent:
            {
              auto& packet = header->data<gfxpacket::PrepareForPresent>();
              ViewResourceHandle viewhandle{};
              viewhandle.resource = packet.texture.rawValue;
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
        buffers[buffers.size()-1]->insert<gfxpacket::ReleaseFromQueue>();
        auto drawIndex = solver.addDrawCall();
        for (auto&& rel : releases)
        {
          ResourceHandle h;
          h.id = rel.id;
          h.type = rel.type;
          ViewResourceHandle view;
          view.resource = h.rawValue;
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
      solver.localBarrierPass1(isFirstList);
      timing.barrierSolveLocal.stop();
    }

    void DeviceGroupData::globalPassBarrierSolve(CommandListTiming& timing, BarrierSolver& solver) {
      HIGAN_CPU_FUNCTION_SCOPE();
      timing.barrierSolveGlobal.start();
      solver.globalBarrierPass2();
      timing.barrierSolveGlobal.stop();
    }

    void DeviceGroupData::fillNativeList(std::shared_ptr<CommandBufferImpl>& nativeList, VirtualDevice& vdev, MemView<CommandBuffer*>& buffers, BarrierSolver& solver, CommandListTiming& timing) {
      HIGAN_CPU_FUNCTION_SCOPE();
      timing.fillNativeList.start();
      nativeList->fillWith(vdev.device, buffers, solver);
      timing.fillNativeList.stop();
    }

    vector<FirstUseResource> DeviceGroupData::checkQueueDependencies(vector<PreparedCommandlist>& lists) {
      HIGAN_CPU_FUNCTION_SCOPE();
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
          IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "first seen Buffer %d %s\n", id, toString(list.type));
          fur.emplace_back(FirstUseResource{list.device, ResourceType::Buffer, id, list.type});
        });
        seenB.add(fSeen);
        fSeen = list.requirementsTex.exceptFields(seenT);
        fSeen.foreach([&](int id){
          IF_QUEUE_DEPENDENCY_DEBUG HIGAN_LOGi( "first seen Texture %d %s\n", id, toString(list.type));
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
    vector<PreparedCommandlist> DeviceGroupData::prepareNodes(vector<CommandGraphNode>& nodes, bool singleThreaded) {
      HIGAN_CPU_FUNCTION_SCOPE();
      vector<PreparedCommandlist> gpus; // one list per gpu, added to lists once ready.
      gpus.resize(m_devices.size(), PreparedCommandlist{});
      vector<PreparedCommandlist> lists;
      {
        if (globalconfig::graphics::GraphicsEnableShaderDebug && m_shaderDebugReadbacks.size() < 10)
        {
          auto handle = m_handles.allocateResource(ResourceType::ReadbackBuffer);
          handle.setGpuId(nodes.back().gpuId);
          auto promise = ReadbackPromise({sharedHandle(handle), std::make_shared<std::promise<ReadbackData>>()});
          m_devices[nodes.back().gpuId].device->readbackBuffer(handle, HIGANBANA_SHADER_DEBUG_WIDTH);
          nodes.back().list->list.insert<gfxpacket::ReadbackShaderDebug>(handle);
          m_shaderDebugReadbacks.emplace_back(promise.future());
          nodes.back().m_readbackPromises.emplace_back(promise);
        }
      }
      size_t allListSize = 0;
      for (auto&& list : nodes) {
        allListSize += list.list->list.sizeBytes();
      }
      auto splitSize = std::max(allListSize / 32, static_cast<size_t>(higanbana::globalconfig::graphics::GraphicsHowManyBytesBeforeNewCommandBuffer));
      /*if (splitSize > 2 * 1024 * 1024ull) {
        splitSize = std::max(allListSize / 16ull, 2*1024*1024ull);
      }*/

      vector<ResourceHandle> writtenSharedResources;

      int nodesSize = static_cast<int>(nodes.size());
      int i = 0;
      while (i < nodesSize)
      {
        auto* node = &nodes[i];
        bool finished = false;
        auto& plist = gpus[node->gpuId];
        if (plist.initialized && !node->consumesSharedResource.empty()) {
          lists.emplace_back(std::move(plist));
          plist = {};
        }
        plist.nodeIndex = i;
        if (!plist.initialized) {
          plist.initialized = true;
          plist.device = node->gpuId;
          plist.type = node->type;
          plist.timing.type = node->type;
        }
        if (node->type == plist.type && (singleThreaded || plist.bytesOfList < splitSize)) {
          i++; // only allowed here to progress list as all nodes have to be processed.
          auto addedNodeSize = node->list->list.sizeBytes(); 
          plist.bytesOfList += addedNodeSize;
          plist.requiredConstantMemory += node->usedConstantMemory;
          plist.buffers.emplace_back(&node->list->list);
          //HIGAN_LOGi("%d node %zu bytes\n", i, plist.buffers.back().sizeBytes());
          plist.requirementsBuf = plist.requirementsBuf.unionFields(node->refBuf());
          plist.requirementsTex = plist.requirementsTex.unionFields(node->refTex());
          plist.readbacks.insert(plist.readbacks.end(), node->m_readbackPromises.begin(), node->m_readbackPromises.end());
          plist.timing.nodes.emplace_back(node->timing);
          if (!plist.acquireSema)
          {
            plist.acquireSema = node->acquireSemaphore;
          }
          if (!plist.presents)
          {
            plist.presents = node->preparesPresent;
          }
          for (auto& reads : node->consumesSharedResource) {
            for (auto& writes : writtenSharedResources) {
              if (reads.id == writes.id) {
                // todo: insert fence wait for shared fence.
                //HIGAN_LOGi("preparenodes gpu:%d depends from gpu:%d !\n", node->gpuId, writes.ownerGpuId());
                plist.sharedWaits.push_back(writes.ownerGpuId());
              }
            }
          }
          for (auto& write : node->producesSharedResources) {
            write.setGpuId(node->gpuId);
            writtenSharedResources.push_back(write);
            // todo: insert fence signal to shared fence.
            //HIGAN_LOGi("preparenodes gpu:%d produces something !\n", node->gpuId);
            plist.sharedSignals.push_back(node->gpuId);
            finished = true;
          }
        }
        else
        {
          finished = true;
        }
        bool seenInLastNodes = false;
        if (!finished) {
          for (int k = plist.nodeIndex+1; k < nodesSize; ++k)
            if (nodes[k].gpuId == plist.device) {
              seenInLastNodes = true;
              break;
            }
        }
        if (finished || !seenInLastNodes) {

          lists.emplace_back(std::move(plist));
          plist = {};
        }
      }
      int lsize = lists.size();
      lists[lsize - 1].isLastList = true;
      return lists;
    }

    void DeviceGroupData::returnResouresToOriginalQueues(vector<PreparedCommandlist>& lists, vector<backend::FirstUseResource>& firstUsageSeen) {
      HIGAN_CPU_FUNCTION_SCOPE();
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

    void DeviceGroupData::handleQueueTransfersWithinRendergraph(vector<PreparedCommandlist>& lists, vector<backend::FirstUseResource>& firstUsageSeen) {
      HIGAN_CPU_FUNCTION_SCOPE();
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

    // one liveCommandBuffer for each deivce and queue.
    deque<LiveCommandBuffer2> DeviceGroupData::makeLiveCommandBuffers(vector<PreparedCommandlist>& lists, uint64_t submitID) {
      HIGAN_CPU_FUNCTION_SCOPE();
      deque<LiveCommandBuffer2> readyLists;
      int listID = 0;
      
      LiveCommandBuffer2 buffer{};
      buffer.queue = lists[0].type;
      buffer.deviceID = lists[0].device;

      for (auto&& list : lists)
      {
        HIGAN_CPU_BRACKET("list in lists");
        if (list.type != buffer.queue || list.device != buffer.deviceID)
        {
          HIGAN_CPU_BRACKET("readyLists.emplace_back");
          HIGAN_ASSERT(!buffer.listIDs.empty(), "wtf!");
          readyLists.emplace_back(std::move(buffer));
          buffer = LiveCommandBuffer2{};

          if (list.type == QueueType::Graphics || list.type == QueueType::Compute) {
            // need dma list for constants
            //buffer.dmaListConstants = vdev.device->createDMAList();
          }
        }
        std::shared_ptr<CommandBufferImpl> nativeList;
        buffer.submitID = submitID;
        buffer.listTiming.push_back(list.timing);
        buffer.queue = list.type;
        buffer.listTiming.back().cpuBackendTime.start();
        buffer.fence = nullptr;

        auto& vdev = m_devices[list.device];

        {
          HIGAN_CPU_BRACKET("createNativeList");
          switch (list.type)
          {
          case QueueType::Dma:
            nativeList = vdev.device->createDMAList();
            break;
          case QueueType::Compute:
            nativeList = vdev.device->createComputeList();
            nativeList->reserveConstants(list.requiredConstantMemory);
            break;
          case QueueType::Graphics:
          default:
            nativeList = vdev.device->createGraphicsList();
            nativeList->reserveConstants(list.requiredConstantMemory);
          }
        }

        buffer.deviceID = list.device;
        buffer.started.push_back(m_seqTracker.next());
        buffer.lists.emplace_back(nativeList);
        buffer.listIDs.push_back(listID);
        buffer.readbacks.emplace_back(std::move(list.readbacks));

        listID++;
      }
      HIGAN_CPU_BRACKET("readyLists.emplace_back");
      readyLists.emplace_back(std::move(buffer));
      return readyLists;
    }

    void DeviceGroupData::submit(std::optional<Swapchain> swapchain, CommandGraph& graph, ThreadedSubmission multithreaded) {
      HIGAN_CPU_FUNCTION_SCOPE();
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
          HIGAN_CPU_BRACKET("addNodes");
          timing.addNodes.start();
          lists = prepareNodes(nodes, false);
          timing.addNodes.stop();
        }

        {
          HIGAN_CPU_BRACKET("GraphSolve");
          timing.graphSolve.start();
          auto firstUsageSeen = checkQueueDependencies(lists);
          returnResouresToOriginalQueues(lists, firstUsageSeen);
          handleQueueTransfersWithinRendergraph(lists, firstUsageSeen);
          timing.graphSolve.stop();
        }

        timing.fillCommandLists.start();

        auto readyLists = makeLiveCommandBuffers(lists, timing.id);
        timing.listsCount = lists.size();

        std::future<void> gcComplete;
        {
          HIGAN_CPU_BRACKET("launch gc task");
          gcComplete = std::async(std::launch::async, [&]
          {
            gc();
          });
        }

        vector<std::shared_ptr<BarrierSolver>> solvers;
        solvers.resize(lists.size());

        std::for_each(std::execution::par_unseq, std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list) {
          HIGAN_CPU_BRACKET("OuterLoopFirstPass");
          int offset = list.listIDs[0];
          std::for_each(std::execution::par_unseq, std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            HIGAN_CPU_BRACKET("InnerLoopFirstPass");
            auto& vdev = m_devices[list.deviceID];
            {
              HIGAN_CPU_BRACKET("BarrierSolver creation");
              solvers[id] = std::make_shared<BarrierSolver>(vdev.m_bufferStates, vdev.m_textureStates);
            }
            auto& solver = *solvers[id];
            auto& buffer = lists[id];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, list.listTiming[id - offset], solver, list.readbacks[id - offset], id == offset);
          });
        });
        
        {
          HIGAN_CPU_BRACKET("globalPass");
          for (auto&& list : readyLists)
          {
            int offset = list.listIDs[0];
            for (auto&& id : list.listIDs)
            {
              auto& solver = *solvers[id];
              // this is order dependant
              globalPassBarrierSolve(list.listTiming[id - offset], solver);
              // also parallel
            }
          }
        }

        std::for_each(std::execution::par_unseq, std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list)
        {
          int offset = list.listIDs[0];
          HIGAN_CPU_BRACKET("OuterLoopFillNativeList");
          std::for_each(std::execution::par_unseq, std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            auto& buffer = lists[id];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            auto& vdev = m_devices[list.deviceID];
            auto& solver = *solvers[id];
            fillNativeList(list.lists[id-offset], vdev, buffersView, solver, list.listTiming[id - offset]);
            list.listTiming[id - offset].cpuBackendTime.stop();
          });
        });

        timing.fillCommandLists.stop();

        timing.submitSolve.start();
        // submit can be "multithreaded" also in the order everything finished, but not in current shape where readyLists is modified.
        //for (auto&& list : lists)
        gcComplete.wait();
        HIGAN_CPU_BRACKET("Submit Lists");
        while(!readyLists.empty())
        {
          LiveCommandBuffer2 buffer = std::move(readyLists.front());
          readyLists.pop_front();

          auto& vdev = m_devices[buffer.deviceID];
          buffer.dmaValue = 0;
          buffer.gfxValue = 0;
          buffer.cptValue = 0;
          std::optional<std::shared_ptr<FenceImpl>> viewToFence;
          std::optional<TimelineSemaphoreInfo> timelineGfx;
          std::optional<TimelineSemaphoreInfo> timelineCompute;
          std::optional<TimelineSemaphoreInfo> timelineDma;
          vector<int> sharedSignals;
          vector<int> sharedWaits;
          for (auto&& id : buffer.listIDs)
          {
            auto& list = lists[id];

            if (list.waitGraphics)
            {
              timelineGfx = TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue};
            }
            if (list.waitCompute)
            {
              timelineCompute = TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue};
            }
            if (list.waitDMA)
            {
              timelineDma = TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue};
            }
            for (auto& wait : list.sharedWaits) {
              bool has = false;
              for (auto&& it : sharedWaits)
                if (it == wait)
                  has = true;
              if (!has)
                sharedWaits.push_back(wait);
            }
            for (auto& signal : list.sharedSignals) {
              bool has = false;
              for (auto&& it : sharedSignals)
                if (it == signal)
                  has = true;
              if (!has)
                sharedSignals.push_back(signal);
            }

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
            
            // this could be removed when vulkan debug layers work
            if (list.isLastList || !buffer.readbacks.empty())
            {
              buffer.fence = vdev.device->createFence();
            }

            if (buffer.fence)
            {
              viewToFence = buffer.fence;
            }
            // as fences arent needed with timelines semaphores

            for (auto&& timing : buffer.listTiming)
            {
              timing.fromSubmitToFence.start();
            }
          }

          vector<TimelineSemaphoreInfo> waitInfos;
          if (timelineGfx) waitInfos.push_back(timelineGfx.value());
          if (timelineCompute) waitInfos.push_back(timelineCompute.value());
          if (timelineDma) waitInfos.push_back(timelineDma.value());

          for (auto&& wait : sharedWaits) {
            waitInfos.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[wait].get(), m_devices[wait].sharedValue});
          }
          vector<TimelineSemaphoreInfo> signalTimelines;
          for (auto&& signal : sharedSignals) {
            m_devices[signal].sharedValue++;
            signalTimelines.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[signal].get(), m_devices[signal].sharedValue});
          }

          switch (buffer.queue)
          {
          case QueueType::Dma:
            ++vdev.dmaQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue});
            buffer.dmaValue = vdev.dmaQueue;
            vdev.device->submitDMA(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            ++vdev.cptQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue});
            buffer.cptValue = vdev.cptQueue;
            vdev.device->submitCompute(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
#if 0
            {
              for (int i = 0; i < buffer.lists.size(); i++)
              {
                if (i == 0)
                  vdev.device->submitGraphics(buffer.lists[i], buffer.wait, {}, {});
                else if (i == buffer.lists.size()-1)
                  vdev.device->submitGraphics(buffer.lists[i], {}, buffer.signal, viewToFence);
                else
                  vdev.device->submitGraphics(buffer.lists[i], {}, {}, {});
              }
            }
#else
            ++vdev.gfxQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue});
            buffer.gfxValue = vdev.gfxQueue;
            vdev.device->submitGraphics(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
#endif
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
        }
        timing.submitSolve.stop();
      }
      for (auto&& list : nodes) {
        m_commandBuffers.free(std::move(list.list->list));
      }
      timing.submitCpuTime.stop();
      timeOnFlightSubmits.push_back(timing);
      if (graph.m_sequence != InvalidSeqNum)
      {
        m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
      }
    }

    void DeviceGroupData::garbageCollection() {
      HIGAN_CPU_FUNCTION_SCOPE();
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
          auto ownerGpuId = handle.resourceHandle().ownerGpuId();
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
          if (handle.type == ResourceType::ReadbackBuffer)
          {
            device.device->unmapReadback(handle);
            device.device->releaseHandle(handle);
          }
          else
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

    void DeviceGroupData::present(Swapchain & swapchain, int backbufferIndex) {
      HIGAN_CPU_FUNCTION_SCOPE();
      std::lock_guard<std::mutex> guard(m_presentMutex);
      m_asyns.emplace_back(std::async(std::launch::async, [&, sc = swapchain.impl(), index = backbufferIndex]{
        m_devices[SwapchainDeviceID].device->present(sc, sc->renderSemaphore(), index);
      }));
    }

    css::Task<void> DeviceGroupData::presentAsync(Swapchain& swapchain, int backbufferIndex) {
      HIGAN_CPU_FUNCTION_SCOPE();
      /*
      std::lock_guard<std::mutex> guard(m_presentMutex);
      m_asyns.emplace_back(std::async(std::launch::async, [&, sc = swapchain.impl(), index = backbufferIndex]{
        m_devices[SwapchainDeviceID].device->present(sc, sc->renderSemaphore(), index);
      }));*/
      co_return;
    }
    void DeviceGroupData::submitExperimental(std::optional<Swapchain> swapchain, CommandGraph& graph, ThreadedSubmission multithreaded) {
      HIGAN_CPU_FUNCTION_SCOPE();
      std::launch policy = std::launch::async | std::launch::deferred;
      if (multithreaded == ThreadedSubmission::Sequenced)
        policy = std::launch::deferred;
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
          HIGAN_CPU_BRACKET("addNodes");
          timing.addNodes.start();
          lists = prepareNodes(nodes, false);
          timing.addNodes.stop();
        }

        {
          HIGAN_CPU_BRACKET("GraphSolve");
          timing.graphSolve.start();
          auto firstUsageSeen = checkQueueDependencies(lists);
          returnResouresToOriginalQueues(lists, firstUsageSeen);
          handleQueueTransfersWithinRendergraph(lists, firstUsageSeen);
          timing.graphSolve.stop();
        }

        timing.fillCommandLists.start();

        auto readyLists = makeLiveCommandBuffers(lists, timing.id);
        timing.listsCount = lists.size();

        std::shared_future<void> gcComplete;
        {
          HIGAN_CPU_BRACKET("launch gc task");
          gcComplete = std::async(std::launch::async, [&]
          {
            gc();
          });
        }

        vector<std::shared_ptr<BarrierSolver>> solvers;
        solvers.resize(lists.size());

        vector<vector<std::shared_future<void>>> listsFilled;
        std::optional<std::shared_future<void>> lastGlobalPass;
        std::optional<std::shared_future<void>> lastSubmitPass;

        for (int liveListID = 0; liveListID < readyLists.size(); liveListID++)
        //for (auto&& liveList : readyLists)
        {
          int listIdBegin = readyLists[liveListID].listIDs[0];
          vector<std::shared_future<void>> listfills;
          for (auto&& listID : readyLists[liveListID].listIDs)
          {
            std::shared_future<void> localpass = std::async(policy, [&, listIdBegin, liveListID, listID](){
              HIGAN_CPU_BRACKET("localpass");
              auto& liveList = readyLists[liveListID];
              auto& vdev = m_devices[liveList.deviceID];
              {
                HIGAN_CPU_BRACKET("BarrierSolver creation");
                solvers[listID] = std::make_shared<BarrierSolver>(vdev.m_bufferStates, vdev.m_textureStates);
              }
              auto& solver = *solvers[listID];
              auto& buffer = lists[listID];
              auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
              firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, liveList.listTiming[listID - listIdBegin], solver, liveList.readbacks[listID - listIdBegin], listID == listIdBegin);
            });
            std::shared_future<void> globalpass = std::async(policy, [&, localpass, lastGlobalPass, listIdBegin, liveListID, listID](){
              if (lastGlobalPass)
              {
                HIGAN_CPU_BRACKET("waiting previous globalpass");
                lastGlobalPass->wait();
              }
              {
                HIGAN_CPU_BRACKET("waiting own localpass");
                localpass.wait();
              }
              HIGAN_CPU_BRACKET("globalPass");
              std::lock_guard<std::mutex> guard(m_presentMutex);
              auto& solver = *solvers[listID];
              // this is order dependant
              auto& liveList = readyLists[liveListID];
              globalPassBarrierSolve(liveList.listTiming[listID - listIdBegin], solver);
            });
            lastGlobalPass = globalpass;
            std::shared_future<void> listfill = std::async(policy, [&, globalpass, listIdBegin, liveListID, listID](){
              {
                HIGAN_CPU_BRACKET("waiting own global pass to finish");
                globalpass.wait();
              }

              HIGAN_CPU_BRACKET("filling list");
              auto& buffer = lists[listID];
              auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
              auto& liveList = readyLists[liveListID];
              auto& vdev = m_devices[liveList.deviceID];
              auto& solver = *solvers[listID];
              fillNativeList(liveList.lists[listID - listIdBegin], vdev, buffersView, solver, liveList.listTiming[listID - listIdBegin]);
              liveList.listTiming[listID - listIdBegin].cpuBackendTime.stop();
            });

            /////////////////////////SUBMIT////////////////////////////////////////

            std::shared_future<void> submitPass = std::async(policy, [&, listfill, lastSubmitPass, gcComplete, listIdBegin, liveListID, listID](){
              if (lastSubmitPass)
              {
                HIGAN_CPU_BRACKET("waiting previous submit");
                lastSubmitPass->wait();
              }
              {
                HIGAN_CPU_BRACKET("waiting own list fill");
                listfill.wait();
              }
              {
                HIGAN_CPU_BRACKET("waiting gc completion");
                gcComplete.wait();
              }
              HIGAN_CPU_BRACKET("submit");
              std::lock_guard<std::mutex> guard(m_presentMutex);

              auto& liveList = readyLists[liveListID];
              auto isFirstList = listID == liveList.listIDs.front();
              auto isLastList = listID == liveList.listIDs.back();

              auto& vdev = m_devices[liveList.deviceID];
              if (isFirstList)
              {
                liveList.dmaValue = 0;
                liveList.gfxValue = 0;
                liveList.cptValue = 0;
              }
              vector<int> sharedSignals;
              vector<int> sharedWaits;
              MemView<std::shared_ptr<SemaphoreImpl>> wait;
              {
                auto& list = lists[listID];

                if (list.waitGraphics) {
                  liveList.timelineGfx = TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue};
                }
                if (list.waitCompute) {
                  liveList.timelineCompute = TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue};
                }
                if (list.waitDMA) {
                  liveList.timelineDma = TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue};
                }
                for (auto& wait : list.sharedWaits) {
                  bool has = false;
                  for (auto&& it : sharedWaits)
                    if (it == wait)
                      has = true;
                  if (!has)
                    sharedWaits.push_back(wait);
                }
                for (auto& signal : list.sharedSignals) {
                  bool has = false;
                  for (auto&& it : sharedSignals)
                    if (it == signal)
                      has = true;
                  if (!has)
                    sharedSignals.push_back(signal);
                }

                if (list.acquireSema) {
                  liveList.wait.push_back(list.acquireSema);
                  wait = list.acquireSema;
                }
                if (list.presents && swapchain) {
                  auto sc = swapchain.value();
                  auto presenene = sc.impl()->renderSemaphore();
                  if (presenene) {
                    liveList.signal.push_back(presenene);
                  }
                }
                // this could be removed when vulkan debug layers work, timeline semaphores
                if (list.isLastList || !liveList.readbacks.empty()) {
                  liveList.fence = vdev.device->createFence();
                }
                // as fences arent needed with timelines semaphores

                for (auto&& timing : liveList.listTiming) {
                  timing.fromSubmitToFence.start();
                }
              }

              vector<TimelineSemaphoreInfo> waitInfos;
              if (liveList.timelineGfx) {
                waitInfos.push_back(liveList.timelineGfx.value());
                liveList.timelineGfx.reset();
              }
              if (liveList.timelineCompute) {
                waitInfos.push_back(liveList.timelineCompute.value());
                liveList.timelineCompute.reset();
              } 
              if (liveList.timelineDma) {
                waitInfos.push_back(liveList.timelineDma.value());
                liveList.timelineDma.reset();
              }

              MemView<std::shared_ptr<SemaphoreImpl>> signal;
              std::optional<std::shared_ptr<FenceImpl>> viewToFence;
              if (isLastList)
              {
                signal = liveList.signal;
                if (liveList.fence)
                  viewToFence = liveList.fence;
              }
              for (auto&& wait : sharedWaits) {
                waitInfos.push_back(TimelineSemaphoreInfo{m_devices[liveList.deviceID].sharedTimelines[wait].get(), m_devices[wait].sharedValue});
              }
              vector<TimelineSemaphoreInfo> signalTimelines;
              for (auto&& signal : sharedSignals) {
                m_devices[signal].sharedValue++;
                signalTimelines.push_back(TimelineSemaphoreInfo{m_devices[liveList.deviceID].sharedTimelines[signal].get(), m_devices[signal].sharedValue});
              }
              switch (liveList.queue)
              {
              case QueueType::Dma:
                ++vdev.dmaQueue;
                signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue});
                liveList.dmaValue = vdev.dmaQueue;
                vdev.device->submitDMA(liveList.lists[listID], wait, signal, waitInfos, signalTimelines, viewToFence);
                break;
              case QueueType::Compute:
                ++vdev.cptQueue;
                signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue});
                liveList.cptValue = vdev.cptQueue;
                vdev.device->submitCompute(liveList.lists[listID], wait, signal, waitInfos, signalTimelines, viewToFence);
                break;
              case QueueType::Graphics:
              default:
    #if 0
                {
                  for (int i = 0; i < buffer.lists.size(); i++)
                  {
                    if (i == 0)
                      vdev.device->submitGraphics(buffer.lists[i], buffer.wait, {}, {});
                    else if (i == buffer.lists.size()-1)
                      vdev.device->submitGraphics(buffer.lists[i], {}, buffer.signal, viewToFence);
                    else
                      vdev.device->submitGraphics(buffer.lists[i], {}, {}, {});
                  }
                }
    #else
                ++vdev.gfxQueue;
                signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue});
                liveList.gfxValue = vdev.gfxQueue;
                vdev.device->submitGraphics(liveList.lists[listID], wait, signal, waitInfos, signalTimelines, viewToFence);
    #endif
              }
            });
            lastSubmitPass = submitPass;
            listfills.emplace_back(submitPass);
          }
          listsFilled.emplace_back(std::move(listfills));
        }

        timing.fillCommandLists.stop();

        timing.submitSolve.start();
        // submit can be "multithreaded" also in the order everything finished, but not in current shape where readyLists is modified.
        //for (auto&& list : lists)
        HIGAN_CPU_BRACKET("waiting submit Lists");
        for (auto&& buffer : readyLists)
        {
          {
            HIGAN_CPU_BRACKET("waiting lists to be submitted");
            for (auto&& listFill : listsFilled.back())
            {
              listFill.wait();
            }
            listsFilled.pop_back();
          }
          auto& vdev = m_devices[buffer.deviceID];

          switch (buffer.queue)
          {
          case QueueType::Dma:
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
        }
        timing.submitSolve.stop();
      }
      for (auto&& list : nodes) {
        m_commandBuffers.free(std::move(list.list->list));
      }
      timing.submitCpuTime.stop();
      timeOnFlightSubmits.push_back(timing);
      if (graph.m_sequence != InvalidSeqNum)
      {
        m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
      }
    }

    void DeviceGroupData::submitST(std::optional<Swapchain> swapchain, CommandGraph& graph) {
      HIGAN_CPU_FUNCTION_SCOPE();
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
          HIGAN_CPU_BRACKET("addNodes");
          timing.addNodes.start();
          lists = prepareNodes(nodes, true);
          timing.addNodes.stop();
        }

        {
          HIGAN_CPU_BRACKET("GraphSolve");
          timing.graphSolve.start();
          auto firstUsageSeen = checkQueueDependencies(lists);
          returnResouresToOriginalQueues(lists, firstUsageSeen);
          handleQueueTransfersWithinRendergraph(lists, firstUsageSeen);
          timing.graphSolve.stop();
        }

        timing.fillCommandLists.start();

        auto readyLists = makeLiveCommandBuffers(lists, timing.id);
        timing.listsCount = lists.size();

        //std::future<void> gcComplete;
        {
          //HIGAN_CPU_BRACKET("launch gc task");
          //gcComplete = std::async(std::launch::async, [&]
          //{
            gc();
          //});
        }

        vector<std::shared_ptr<BarrierSolver>> solvers;
        solvers.resize(lists.size());

        std::for_each(std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list) {
          HIGAN_CPU_BRACKET("OuterLoopFirstPass");
          int offset = list.listIDs[0];
          std::for_each(std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            HIGAN_CPU_BRACKET("InnerLoopFirstPass");
            auto& vdev = m_devices[list.deviceID];
            {
              HIGAN_CPU_BRACKET("BarrierSolver creation");
              solvers[id] = std::make_shared<BarrierSolver>(vdev.m_bufferStates, vdev.m_textureStates);
            }
            auto& solver = *solvers[id];
            auto& buffer = lists[id];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, list.listTiming[id - offset], solver, list.readbacks[id - offset], id == offset);
          });
        });
        
        {
          HIGAN_CPU_BRACKET("globalPass");
          for (auto&& list : readyLists)
          {
            int offset = list.listIDs[0];
            for (auto&& id : list.listIDs)
            {
              auto& solver = *solvers[id];
              // this is order dependant
              globalPassBarrierSolve(list.listTiming[id - offset], solver);
              // also parallel
            }
          }
        }

        std::for_each(std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list)
        {
          int offset = list.listIDs[0];
          HIGAN_CPU_BRACKET("OuterLoopFillNativeList");
          std::for_each(std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            auto& buffer = lists[id];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            auto& vdev = m_devices[list.deviceID];
            auto& solver = *solvers[id];
            fillNativeList(list.lists[id-offset], vdev, buffersView, solver, list.listTiming[id - offset]);
            list.listTiming[id - offset].cpuBackendTime.stop();
          });
        });

        timing.fillCommandLists.stop();

        timing.submitSolve.start();
        // submit can be "multithreaded" also in the order everything finished, but not in current shape where readyLists is modified.
        //for (auto&& list : lists)
        //gcComplete.wait();
        HIGAN_CPU_BRACKET("Submit Lists");
        while(!readyLists.empty())
        {
          LiveCommandBuffer2 buffer = std::move(readyLists.front());
          readyLists.pop_front();

          auto& vdev = m_devices[buffer.deviceID];
          buffer.dmaValue = 0;
          buffer.gfxValue = 0;
          buffer.cptValue = 0;
          std::optional<std::shared_ptr<FenceImpl>> viewToFence;
          std::optional<TimelineSemaphoreInfo> timelineGfx;
          std::optional<TimelineSemaphoreInfo> timelineCompute;
          std::optional<TimelineSemaphoreInfo> timelineDma;
          vector<int> sharedSignals;
          vector<int> sharedWaits;
          for (auto&& id : buffer.listIDs)
          {
            auto& list = lists[id];

            if (list.waitGraphics)
            {
              timelineGfx = TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue};
            }
            if (list.waitCompute)
            {
              timelineCompute = TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue};
            }
            if (list.waitDMA)
            {
              timelineDma = TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue};
            }
            for (auto& wait : list.sharedWaits) {
              bool has = false;
              for (auto&& it : sharedWaits)
                if (it == wait)
                  has = true;
              if (!has)
                sharedWaits.push_back(wait);
            }
            for (auto& signal : list.sharedSignals) {
              bool has = false;
              for (auto&& it : sharedSignals)
                if (it == signal)
                  has = true;
              if (!has)
                sharedSignals.push_back(signal);
            }

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
            
            // this could be removed when vulkan debug layers work
            if (list.isLastList || !buffer.readbacks.empty())
            {
              buffer.fence = vdev.device->createFence();
            }

            if (buffer.fence)
            {
              viewToFence = buffer.fence;
            }
            // as fences arent needed with timelines semaphores

            for (auto&& timing : buffer.listTiming)
            {
              timing.fromSubmitToFence.start();
            }
          }

          vector<TimelineSemaphoreInfo> waitInfos;
          if (timelineGfx) waitInfos.push_back(timelineGfx.value());
          if (timelineCompute) waitInfos.push_back(timelineCompute.value());
          if (timelineDma) waitInfos.push_back(timelineDma.value());

          for (auto&& wait : sharedWaits) {
            waitInfos.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[wait].get(), m_devices[wait].sharedValue});
          }
          vector<TimelineSemaphoreInfo> signalTimelines;
          for (auto&& signal : sharedSignals) {
            m_devices[signal].sharedValue++;
            signalTimelines.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[signal].get(), m_devices[signal].sharedValue});
          }
          switch (buffer.queue)
          {
          case QueueType::Dma:
            ++vdev.dmaQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue});
            buffer.dmaValue = vdev.dmaQueue;
            vdev.device->submitDMA(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            ++vdev.cptQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue});
            buffer.cptValue = vdev.cptQueue;
            vdev.device->submitCompute(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
#if 0
            {
              for (int i = 0; i < buffer.lists.size(); i++)
              {
                if (i == 0)
                  vdev.device->submitGraphics(buffer.lists[i], buffer.wait, {}, {});
                else if (i == buffer.lists.size()-1)
                  vdev.device->submitGraphics(buffer.lists[i], {}, buffer.signal, viewToFence);
                else
                  vdev.device->submitGraphics(buffer.lists[i], {}, {}, {});
              }
            }
#else
            ++vdev.gfxQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue});
            buffer.gfxValue = vdev.gfxQueue;
            vdev.device->submitGraphics(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
#endif
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
        }
        timing.submitSolve.stop();
      }
      for (auto&& list : nodes) {
        m_commandBuffers.free(std::move(list.list->list));
      }
      timing.submitCpuTime.stop();
      timeOnFlightSubmits.push_back(timing);
      if (graph.m_sequence != InvalidSeqNum)
      {
        m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
      }
    }
    void DeviceGroupData::submitLBS(LBS& lbs, std::optional<Swapchain> swapchain, CommandGraph& graph, ThreadedSubmission multithreaded) {
      HIGAN_CPU_FUNCTION_SCOPE();
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
          HIGAN_CPU_BRACKET("addNodes");
          timing.addNodes.start();
          lists = prepareNodes(nodes, false);
          timing.addNodes.stop();
        }

        {
          HIGAN_CPU_BRACKET("GraphSolve");
          timing.graphSolve.start();
          auto firstUsageSeen = checkQueueDependencies(lists);
          returnResouresToOriginalQueues(lists, firstUsageSeen);
          handleQueueTransfersWithinRendergraph(lists, firstUsageSeen);
          timing.graphSolve.stop();
        }

        timing.fillCommandLists.start();

        auto readyLists = makeLiveCommandBuffers(lists, timing.id);
        timing.listsCount = lists.size();

        //std::shared_future<void> gcComplete;
        std::string gcComplete = std::string("gccomplete");
        {
          HIGAN_CPU_BRACKET("launch gc task");
          lbs.addTask(gcComplete, [&](size_t)
          {
            gc();
          });
        }

        vector<std::shared_ptr<BarrierSolver>> solvers;
        solvers.resize(lists.size());

        vector<vector<std::string>> listsSubmitted;
        //std::optional<std::shared_future<void>> lastGlobalPass;
        //std::optional<std::shared_future<void>> lastSubmitPass;
        std::string lastGlobalPass;
        std::string lastSubmitPass;

        for (int liveListID = 0; liveListID < readyLists.size(); liveListID++)
        //for (auto&& liveList : readyLists)
        {
          int listIdBegin = readyLists[liveListID].listIDs[0];
          vector<std::string> listfills;
          for (auto&& listID : readyLists[liveListID].listIDs)
          {
            auto local = std::to_string(liveListID) + std::string("localpass") + std::to_string(listID);
            lbs.addTask(desc::Task(local, {}, {}), [&, listIdBegin, liveListID, listID](size_t){
              HIGAN_CPU_BRACKET("localpass");
              auto& liveList = readyLists[liveListID];
              auto& vdev = m_devices[liveList.deviceID];
              {
                HIGAN_CPU_BRACKET("BarrierSolver creation");
                solvers[listID] = std::make_shared<BarrierSolver>(vdev.m_bufferStates, vdev.m_textureStates);
              }
              auto& solver = *solvers[listID];
              auto& buffer = lists[listID];
              auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
              firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, liveList.listTiming[listID - listIdBegin], solver, liveList.readbacks[listID - listIdBegin], listID == listIdBegin);
            });
            auto globalpass = std::to_string(liveListID) + std::string("globalpass") + std::to_string(listID);
            desc::Task globalTask = desc::Task(globalpass, {local}, {});
            if (!lastGlobalPass.empty())
            {
              globalTask = desc::Task(globalpass, {local, lastGlobalPass}, {});
            }

            lbs.addTask(globalTask, [&, listIdBegin, liveListID, listID](size_t){
              HIGAN_CPU_BRACKET("globalPass");
              std::lock_guard<std::mutex> guard(m_presentMutex);
              auto& solver = *solvers[listID];
              // this is order dependant
              auto& liveList = readyLists[liveListID];
              globalPassBarrierSolve(liveList.listTiming[listID - listIdBegin], solver);
            });
            lastGlobalPass = globalpass;

            auto listFill = std::to_string(liveListID) + std::string("listfill") + std::to_string(listID);
            lbs.addTask(desc::Task(listFill, {globalpass}, {}), [&, listIdBegin, liveListID, listID](size_t){
              HIGAN_CPU_BRACKET("filling list");
              auto& buffer = lists[listID];
              auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
              auto& liveList = readyLists[liveListID];
              auto& vdev = m_devices[liveList.deviceID];
              auto& solver = *solvers[listID];
              fillNativeList(liveList.lists[listID - listIdBegin], vdev, buffersView, solver, liveList.listTiming[listID - listIdBegin]);
              liveList.listTiming[listID - listIdBegin].cpuBackendTime.stop();
            });

            /////////////////////////SUBMIT////////////////////////////////////////
            auto submitpass = std::to_string(liveListID) + std::string("submit") + std::to_string(listID);
            desc::Task submitTask = desc::Task(submitpass, {listFill, gcComplete}, {});
            if (!lastSubmitPass.empty())
              submitTask = desc::Task(submitpass, {listFill, gcComplete, lastSubmitPass}, {});

            lbs.addTask(submitTask, [&, listIdBegin, liveListID, listID](size_t){
              HIGAN_CPU_BRACKET("submit");
              std::lock_guard<std::mutex> guard(m_presentMutex);

              auto& liveList = readyLists[liveListID];
              auto isFirstList = listID == liveList.listIDs.front();
              auto isLastList = listID == liveList.listIDs.back();

              auto& vdev = m_devices[liveList.deviceID];
              if (isFirstList)
              {
                liveList.dmaValue = 0;
                liveList.gfxValue = 0;
                liveList.cptValue = 0;
              }
              MemView<std::shared_ptr<SemaphoreImpl>> wait;
              vector<int> sharedSignals;
              vector<int> sharedWaits;
              {
                auto& list = lists[listID];

                if (list.waitGraphics) {
                  liveList.timelineGfx = TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue};
                }
                if (list.waitCompute) {
                  liveList.timelineCompute = TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue};
                }
                if (list.waitDMA) {
                  liveList.timelineDma = TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue};
                }
                for (auto& wait : list.sharedWaits) {
                  bool has = false;
                  for (auto&& it : sharedWaits)
                    if (it == wait)
                      has = true;
                  if (!has)
                    sharedWaits.push_back(wait);
                }
                for (auto& signal : list.sharedSignals) {
                  bool has = false;
                  for (auto&& it : sharedSignals)
                    if (it == signal)
                      has = true;
                  if (!has)
                    sharedSignals.push_back(signal);
                }

                if (list.acquireSema) {
                  liveList.wait.push_back(list.acquireSema);
                  wait = list.acquireSema;
                }
                if (list.presents && swapchain) {
                  auto sc = swapchain.value();
                  auto presenene = sc.impl()->renderSemaphore();
                  if (presenene) {
                    liveList.signal.push_back(presenene);
                  }
                }
                // this could be removed when vulkan debug layers work, timeline semaphores
                if (list.isLastList || !liveList.readbacks.empty()) {
                  liveList.fence = vdev.device->createFence();
                }
                // as fences arent needed with timelines semaphores

                for (auto&& timing : liveList.listTiming) {
                  timing.fromSubmitToFence.start();
                }
              }

              vector<TimelineSemaphoreInfo> waitInfos;
              if (liveList.timelineGfx) {
                waitInfos.push_back(liveList.timelineGfx.value());
                liveList.timelineGfx.reset();
              }
              if (liveList.timelineCompute) {
                waitInfos.push_back(liveList.timelineCompute.value());
                liveList.timelineCompute.reset();
              } 
              if (liveList.timelineDma) {
                waitInfos.push_back(liveList.timelineDma.value());
                liveList.timelineDma.reset();
              }

              MemView<std::shared_ptr<SemaphoreImpl>> signal;
              std::optional<std::shared_ptr<FenceImpl>> viewToFence;
              if (isLastList)
              {
                signal = liveList.signal;
                if (liveList.fence)
                  viewToFence = liveList.fence;
              }
              for (auto&& wait : sharedWaits) {
                waitInfos.push_back(TimelineSemaphoreInfo{m_devices[liveList.deviceID].sharedTimelines[wait].get(), m_devices[wait].sharedValue});
              }
              vector<TimelineSemaphoreInfo> signalTimelines;
              for (auto&& signal : sharedSignals) {
                m_devices[signal].sharedValue++;
                signalTimelines.push_back(TimelineSemaphoreInfo{m_devices[liveList.deviceID].sharedTimelines[signal].get(), m_devices[signal].sharedValue});
              }
              switch (liveList.queue)
              {
              case QueueType::Dma:
                ++vdev.dmaQueue;
                signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue});
                liveList.dmaValue = vdev.dmaQueue;
                vdev.device->submitDMA(liveList.lists[listID], wait, signal, waitInfos, signalTimelines, viewToFence);
                break;
              case QueueType::Compute:
                ++vdev.cptQueue;
                signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue});
                liveList.cptValue = vdev.cptQueue;
                vdev.device->submitCompute(liveList.lists[listID], wait, signal, waitInfos, signalTimelines, viewToFence);
                break;
              case QueueType::Graphics:
              default:
    #if 0
                {
                  for (int i = 0; i < buffer.lists.size(); i++)
                  {
                    if (i == 0)
                      vdev.device->submitGraphics(buffer.lists[i], buffer.wait, {}, {});
                    else if (i == buffer.lists.size()-1)
                      vdev.device->submitGraphics(buffer.lists[i], {}, buffer.signal, viewToFence);
                    else
                      vdev.device->submitGraphics(buffer.lists[i], {}, {}, {});
                  }
                }
    #else
                ++vdev.gfxQueue;
                signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue});
                liveList.gfxValue = vdev.gfxQueue;
                vdev.device->submitGraphics(liveList.lists[listID], wait, signal, waitInfos, signalTimelines, viewToFence);
    #endif
              }
            });
            lastSubmitPass = submitpass;
            listfills.emplace_back(submitpass);
          }
          listsSubmitted.emplace_back(std::move(listfills));
        }

        timing.fillCommandLists.stop();

        timing.submitSolve.start();
        // submit can be "multithreaded" also in the order everything finished, but not in current shape where readyLists is modified.
        //for (auto&& list : lists)
        HIGAN_CPU_BRACKET("waiting submit Lists");
        lbs.sleepTillKeywords({listsSubmitted.back().back()});

        for (auto&& buffer : readyLists)
        {
          auto& vdev = m_devices[buffer.deviceID];

          switch (buffer.queue)
          {
          case QueueType::Dma:
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
        }
        timing.submitSolve.stop();
      }
      for (auto&& list : nodes) {
        m_commandBuffers.free(std::move(list.list->list));
      }
      timing.submitCpuTime.stop();
      timeOnFlightSubmits.push_back(timing);
      if (graph.m_sequence != InvalidSeqNum)
      {
        m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
      }
    }

    template<size_t ppt, typename T, typename Func>
    css::Task<void> parallel_forCSS(T start, T end, Func&& f) {
      size_t size = end - start;
      while (size > 0) {
        if (size > ppt) {
          if (css::s_stealPool->localQueueSize() == 0) {
            size_t splittedSize = size / 2;
            auto a = parallel_forCSS<ppt>(start, end - splittedSize, std::forward<decltype(f)>(f));
            auto b = parallel_forCSS<ppt>(end - splittedSize, end, std::forward<decltype(f)>(f));
            co_await a;
            co_await b;
            co_return;
          }
        }
        size_t doPPTWork = std::min(ppt, size);

        for (T i = start; i != start+doPPTWork; ++i)
          co_await f(*i);
        start += doPPTWork;
        size = end - start;
      }
      co_return;
    }

    template<size_t ppt, typename T, typename Func>
    css::Task<void> parallel_forCSS2(T start, T end, Func&& f) {
      size_t size = end - start;
      while (size > 0) {
        if (size > ppt) {
          if (css::s_stealPool->localQueueSize() == 0) {
            size_t splittedSize = size / 2;
            auto a = parallel_forCSS2<ppt>(start, end - splittedSize, std::forward<decltype(f)>(f));
            auto b = parallel_forCSS2<ppt>(end - splittedSize, end, std::forward<decltype(f)>(f));
            co_await a;
            co_await b;
            co_return;
          }
        }
        size_t doPPTWork = std::min(ppt, size);

        for (T i = start; i != start+doPPTWork; ++i)
          f(*i);
        start += doPPTWork;
        size = end - start;
      }
      co_return;
    }

    css::Task<void> DeviceGroupData::submitCSS(std::optional<Swapchain> swapchain, CommandGraph& graph) {
      HIGAN_CPU_FUNCTION_SCOPE();
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
          HIGAN_CPU_BRACKET("addNodes");
          timing.addNodes.start();
          lists = prepareNodes(nodes, false);
          timing.addNodes.stop();
        }

        {
          HIGAN_CPU_BRACKET("GraphSolve");
          timing.graphSolve.start();
          auto firstUsageSeen = checkQueueDependencies(lists);
          returnResouresToOriginalQueues(lists, firstUsageSeen);
          handleQueueTransfersWithinRendergraph(lists, firstUsageSeen);
          timing.graphSolve.stop();
        }

        timing.fillCommandLists.start();

        auto readyLists = makeLiveCommandBuffers(lists, timing.id);
        timing.listsCount = lists.size();

        css::Task<void> gcComplete = css::async([&]
          {
            gc();
          }
        );

        vector<std::shared_ptr<BarrierSolver>> solvers;
        solvers.resize(lists.size());

        co_await parallel_forCSS<1>(std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list) -> css::Task<void> {
        //std::for_each(std::execution::par_unseq, std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list) {
          HIGAN_CPU_BRACKET("OuterLoopFirstPass");
          int offset = list.listIDs[0];
          co_await parallel_forCSS2<1>(std::begin(list.listIDs), std::end(list.listIDs), [&](int id) {
          //std::for_each(std::execution::par_unseq, std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            HIGAN_CPU_BRACKET("InnerLoopFirstPass");
            auto& vdev = m_devices[list.deviceID];
            {
              HIGAN_CPU_BRACKET("BarrierSolver creation");
              solvers[id] = std::make_shared<BarrierSolver>(vdev.m_bufferStates, vdev.m_textureStates);
            }
            auto& solver = *solvers[id];
            auto& buffer = lists[id];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, list.listTiming[id - offset], solver, list.readbacks[id - offset], id == offset);
          });
          co_return;
        });
        
        {
          HIGAN_CPU_BRACKET("globalPass");
          for (auto&& list : readyLists)
          {
            int offset = list.listIDs[0];
            for (auto&& id : list.listIDs)
            {
              auto& solver = *solvers[id];
              // this is order dependant
              globalPassBarrierSolve(list.listTiming[id - offset], solver);
              // also parallel
            }
          }
        }

        co_await parallel_forCSS<1>(std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list) -> css::Task<void>
        //std::for_each(std::execution::par_unseq, std::begin(readyLists), std::end(readyLists), [&](backend::LiveCommandBuffer2& list)
        {
          int offset = list.listIDs[0];
          HIGAN_CPU_BRACKET("OuterLoopFillNativeList");
          co_await parallel_forCSS2<1>(std::begin(list.listIDs), std::end(list.listIDs), [&](int id) {
          //std::for_each(std::execution::par_unseq, std::begin(list.listIDs), std::end(list.listIDs), [&](int id){
            auto& buffer = lists[id];
            auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
            auto& vdev = m_devices[list.deviceID];
            auto& solver = *solvers[id];
            fillNativeList(list.lists[id-offset], vdev, buffersView, solver, list.listTiming[id - offset]);
            list.listTiming[id - offset].cpuBackendTime.stop();
          });
          co_return;
        });

        timing.fillCommandLists.stop();

        timing.submitSolve.start();
        // submit can be "multithreaded" also in the order everything finished, but not in current shape where readyLists is modified.
        //for (auto&& list : lists)
        if (!gcComplete.is_ready())
          co_await gcComplete;

        HIGAN_CPU_BRACKET("Submit Lists");
        while(!readyLists.empty())
        {
          LiveCommandBuffer2 buffer = std::move(readyLists.front());
          readyLists.pop_front();

          auto& vdev = m_devices[buffer.deviceID];
          buffer.dmaValue = 0;
          buffer.gfxValue = 0;
          buffer.cptValue = 0;
          std::optional<std::shared_ptr<FenceImpl>> viewToFence;
          std::optional<TimelineSemaphoreInfo> timelineGfx;
          std::optional<TimelineSemaphoreInfo> timelineCompute;
          std::optional<TimelineSemaphoreInfo> timelineDma;
          vector<int> sharedSignals;
          vector<int> sharedWaits;
          for (auto&& id : buffer.listIDs)
          {
            auto& list = lists[id];

            if (list.waitGraphics)
            {
              timelineGfx = TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue};
            }
            if (list.waitCompute)
            {
              timelineCompute = TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue};
            }
            if (list.waitDMA)
            {
              timelineDma = TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue};
            }
            for (auto& wait : list.sharedWaits) {
              bool has = false;
              for (auto&& it : sharedWaits)
                if (it == wait)
                  has = true;
              if (!has)
                sharedWaits.push_back(wait);
            }
            for (auto& signal : list.sharedSignals) {
              bool has = false;
              for (auto&& it : sharedSignals)
                if (it == signal)
                  has = true;
              if (!has)
                sharedSignals.push_back(signal);
            }

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
            
            // this could be removed when vulkan debug layers work
            if (list.isLastList || !buffer.readbacks.empty())
            {
              buffer.fence = vdev.device->createFence();
            }

            if (buffer.fence)
            {
              viewToFence = buffer.fence;
            }
            // as fences arent needed with timelines semaphores

            for (auto&& timing : buffer.listTiming)
            {
              timing.fromSubmitToFence.start();
            }
          }

          vector<TimelineSemaphoreInfo> waitInfos;
          if (timelineGfx) waitInfos.push_back(timelineGfx.value());
          if (timelineCompute) waitInfos.push_back(timelineCompute.value());
          if (timelineDma) waitInfos.push_back(timelineDma.value());

          for (auto&& wait : sharedWaits) {
            waitInfos.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[wait].get(), m_devices[wait].sharedValue});
          }
          vector<TimelineSemaphoreInfo> signalTimelines;
          for (auto&& signal : sharedSignals) {
            m_devices[signal].sharedValue++;
            signalTimelines.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[signal].get(), m_devices[signal].sharedValue});
          }

          switch (buffer.queue)
          {
          case QueueType::Dma:
            ++vdev.dmaQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue});
            buffer.dmaValue = vdev.dmaQueue;
            vdev.device->submitDMA(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            ++vdev.cptQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue});
            buffer.cptValue = vdev.cptQueue;
            vdev.device->submitCompute(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
#if 0
            {
              for (int i = 0; i < buffer.lists.size(); i++)
              {
                if (i == 0)
                  vdev.device->submitGraphics(buffer.lists[i], buffer.wait, {}, {});
                else if (i == buffer.lists.size()-1)
                  vdev.device->submitGraphics(buffer.lists[i], {}, buffer.signal, viewToFence);
                else
                  vdev.device->submitGraphics(buffer.lists[i], {}, {}, {});
              }
            }
#else
            ++vdev.gfxQueue;
            signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue});
            buffer.gfxValue = vdev.gfxQueue;
            vdev.device->submitGraphics(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
#endif
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
        }
        timing.submitSolve.stop();
      }
      for (auto&& list : nodes) {
        m_commandBuffers.free(std::move(list.list->list));
      }
      timing.submitCpuTime.stop();
      timeOnFlightSubmits.push_back(timing);
      if (graph.m_sequence != InvalidSeqNum)
      {
        m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
      }
      co_return;
    }

    void DeviceGroupData::submitLiveCommandBuffer2(std::optional<Swapchain> swapchain, vector<PreparedCommandlist>& lists, backend::LiveCommandBuffer2& liveList) {
      HIGAN_CPU_BRACKET("Submit List");
      LiveCommandBuffer2& buffer = liveList;

      auto& vdev = m_devices[buffer.deviceID];
      buffer.dmaValue = 0;
      buffer.gfxValue = 0;
      buffer.cptValue = 0;
      std::optional<std::shared_ptr<FenceImpl>> viewToFence;
      std::optional<TimelineSemaphoreInfo> timelineGfx;
      std::optional<TimelineSemaphoreInfo> timelineCompute;
      std::optional<TimelineSemaphoreInfo> timelineDma;
      vector<int> sharedSignals;
      vector<int> sharedWaits;

      // Workout constant buffers to gpu memory before submit
      // new commandbuffer from device
      if (buffer.queue != QueueType::Dma && vdev.info.gpuConstants) {
        auto list = vdev.device->createDMAList();
        list->beginConstantsDmaList();
        for (auto&& natList : buffer.lists)
        {
          list->addConstants(natList.get());
        }
        list->endConstantsDmaList();

        ++vdev.dmaQueue;
        vector<TimelineSemaphoreInfo> signalTimelines;
        auto tlInfo = TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue};
        signalTimelines.push_back(tlInfo);
        buffer.dmaValue = vdev.dmaQueue;
        vdev.device->submitDMA({list}, {}, {}, {}, signalTimelines, {});
        buffer.dmaListConstants = list;
        timelineDma = tlInfo;
      }

      // end 
      for (auto&& id : buffer.listIDs)
      {
        auto& list = lists[id];

        if (list.waitGraphics)
        {
          timelineGfx = TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue};
        }
        if (list.waitCompute)
        {
          timelineCompute = TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue};
        }
        if (list.waitDMA)
        {
          timelineDma = TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue};
        }
        for (auto& wait : list.sharedWaits) {
          bool has = false;
          for (auto&& it : sharedWaits)
            if (it == wait)
              has = true;
          if (!has)
            sharedWaits.push_back(wait);
        }
        for (auto& signal : list.sharedSignals) {
          bool has = false;
          for (auto&& it : sharedSignals)
            if (it == signal)
              has = true;
          if (!has)
            sharedSignals.push_back(signal);
        }

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
        
        // this could be removed when vulkan debug layers work
        if (list.isLastList || !buffer.readbacks.empty())
        {
          buffer.fence = vdev.device->createFence();
        }

        if (buffer.fence)
        {
          viewToFence = buffer.fence;
        }
        // as fences arent needed with timelines semaphores

        for (auto&& timing : buffer.listTiming)
        {
          timing.fromSubmitToFence.start();
        }
      }

      vector<TimelineSemaphoreInfo> waitInfos;
      if (timelineGfx) waitInfos.push_back(timelineGfx.value());
      if (timelineCompute) waitInfos.push_back(timelineCompute.value());
      if (timelineDma) waitInfos.push_back(timelineDma.value());

      for (auto&& wait : sharedWaits) {
        waitInfos.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[wait].get(), m_devices[wait].sharedValue});
      }
      vector<TimelineSemaphoreInfo> signalTimelines;
      for (auto&& signal : sharedSignals) {
        m_devices[signal].sharedValue++;
        signalTimelines.push_back(TimelineSemaphoreInfo{m_devices[buffer.deviceID].sharedTimelines[signal].get(), m_devices[signal].sharedValue});
      }

      switch (buffer.queue)
      {
      case QueueType::Dma:
        ++vdev.dmaQueue;
        signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineDma.get(), vdev.dmaQueue});
        buffer.dmaValue = vdev.dmaQueue;
        vdev.device->submitDMA(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
        break;
      case QueueType::Compute:
        ++vdev.cptQueue;
        signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineCompute.get(), vdev.cptQueue});
        buffer.cptValue = vdev.cptQueue;
        vdev.device->submitCompute(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
        break;
      case QueueType::Graphics:
      default:
#if 0
        {
          for (int i = 0; i < buffer.lists.size(); i++)
          {
            if (i == 0)
              vdev.device->submitGraphics(buffer.lists[i], buffer.wait, {}, {});
            else if (i == buffer.lists.size()-1)
              vdev.device->submitGraphics(buffer.lists[i], {}, buffer.signal, viewToFence);
            else
              vdev.device->submitGraphics(buffer.lists[i], {}, {}, {});
          }
        }
#else
        ++vdev.gfxQueue;
        signalTimelines.push_back(TimelineSemaphoreInfo{vdev.timelineGfx.get(), vdev.gfxQueue});
        buffer.gfxValue = vdev.gfxQueue;
        vdev.device->submitGraphics(buffer.lists, buffer.wait, buffer.signal, waitInfos, signalTimelines, viewToFence);
#endif
      }
    }

    css::Task<void> DeviceGroupData::finalPass2(css::Task<void>* previousFinalPass, css::Task<void>* gcDone, std::optional<Swapchain> swapchain, vector<PreparedCommandlist>& lists, backend::LiveCommandBuffer2& liveList, std::shared_ptr<BarrierSolver>& solver, int listID, int listIdBegin) {
      {
        std::string fnlpass = "compile list ";
        fnlpass += std::to_string(listID);
        HIGAN_CPU_BRACKET(fnlpass.c_str());
        auto& buffer = lists[listID];
        auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
        auto& vdev = m_devices[liveList.deviceID];
        std::shared_ptr<CommandBufferImpl>& nativeList = liveList.lists[listID - listIdBegin];
        fillNativeList(nativeList, vdev, buffersView, *solver, liveList.listTiming[listID - listIdBegin]);
        liveList.listTiming[listID - listIdBegin].cpuBackendTime.stop();
      }
      // wait previous submit
      if (previousFinalPass && !previousFinalPass->is_ready()){
        HIGAN_CPU_BRACKET("wait prev compilation(busy wait?)");
        co_await (*previousFinalPass);
      }

      // submit
      if (listID == liveList.listIDs.back()) {
        // wait GC
        if (!gcDone->is_ready())
          co_await (*gcDone);
        
        submitLiveCommandBuffer2(swapchain, lists, liveList);
      }
      // can submit if last one
      co_return;
    }

    css::Task<std::shared_ptr<css::Task<void>>> DeviceGroupData::localPass2(css::Task<std::shared_ptr<css::Task<void>>>* before, css::Task<void>* gcDone, std::optional<Swapchain> swapchain, vector<PreparedCommandlist>& lists, backend::LiveCommandBuffer2& liveList, std::shared_ptr<BarrierSolver>& solver, int listID, int listIdBegin) {
      {
        std::string localpss = "resolve barriers ";
        localpss += std::to_string(listID);
        HIGAN_CPU_BRACKET(localpss.c_str());
        auto& buffer = lists[listID];
        auto& vdev = m_devices[liveList.deviceID];
        {
          HIGAN_CPU_BRACKET("BarrierSolver creation");
          solver = std::make_shared<BarrierSolver>(vdev.m_bufferStates, vdev.m_textureStates);
        }
        auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
        firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, liveList.listTiming[listID - listIdBegin], *solver, liveList.readbacks[listID - listIdBegin], listID == listIdBegin);
      }

      // wait for previous local pass to complete before continuing to access global datas
      {
        HIGAN_CPU_BRACKET("wait-in-line(busy wait?)");
        if (before && !before->is_ready())
          co_await (*before);
      }
      {
        HIGAN_CPU_BRACKET("global barriers");
        // this is order dependant
        globalPassBarrierSolve(liveList.listTiming[listID - listIdBegin], *solver);
      }
      co_return std::make_shared<css::Task<void>>(finalPass2((before != nullptr ? before->get().get() : nullptr), gcDone, swapchain, lists, liveList, solver, listID, listIdBegin));
    }

    css::Task<void> DeviceGroupData::localPass(css::Task<void>* previousLocalPass, PreparedCommandlist& buffer, std::shared_ptr<BarrierSolver>& solver, backend::LiveCommandBuffer2& liveList, int listID, int listIdBegin) {
      std::string localpss = "local pass ";
      localpss += std::to_string(listID);
      HIGAN_CPU_BRACKET(localpss.c_str());
      auto& vdev = m_devices[liveList.deviceID];
      {
        HIGAN_CPU_BRACKET("BarrierSolver creation");
        solver = std::make_shared<BarrierSolver>(vdev.m_bufferStates, vdev.m_textureStates);
      }
      auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
      firstPassBarrierSolve(vdev, buffersView, buffer.type, buffer.acquire, buffer.release, liveList.listTiming[listID - listIdBegin], *solver, liveList.readbacks[listID - listIdBegin], listID == listIdBegin);

      // wait for previous local pass to complete before continuing to access global data
      if (previousLocalPass && !previousLocalPass->is_ready())
        co_await (*previousLocalPass);
      HIGAN_CPU_BRACKET("global pass");
      // this is order dependant
      globalPassBarrierSolve(liveList.listTiming[listID - listIdBegin], *solver);
      co_return;
    }

    css::Task<void> DeviceGroupData::finalPass(css::Task<void>* localPass, css::Task<void>* previousFinalPass, css::Task<void>* gcDone, std::optional<Swapchain> swapchain, vector<PreparedCommandlist>& lists, backend::LiveCommandBuffer2& liveList, std::shared_ptr<BarrierSolver>& solver, int listID, int listIdBegin) {
      // wait for own local pass to finish before filling commandbuffers
      co_await (*localPass);
      {
        std::string fnlpass = "final pass ";
        fnlpass += std::to_string(listID);
        HIGAN_CPU_BRACKET(fnlpass.c_str());
        auto& buffer = lists[listID];
        auto buffersView = makeMemView(buffer.buffers.data(), buffer.buffers.size());
        auto& vdev = m_devices[liveList.deviceID];
        std::shared_ptr<CommandBufferImpl>& nativeList = liveList.lists[listID - listIdBegin];
        fillNativeList(nativeList, vdev, buffersView, *solver, liveList.listTiming[listID - listIdBegin]);
        liveList.listTiming[listID - listIdBegin].cpuBackendTime.stop();
      }

      // wait previous submit
      if (previousFinalPass && !previousFinalPass->is_ready())
        co_await (*previousFinalPass);

      // submit
      if (listID == liveList.listIDs.back()) {
        // wait GC
        if (!gcDone->is_ready())
          co_await (*gcDone);

        submitLiveCommandBuffer2(swapchain, lists, liveList);
      }
      co_return;
    }

    css::Task<void> DeviceGroupData::submitCSSExp(std::optional<Swapchain> swapchain, CommandGraph& graph) {
      HIGAN_CPU_BRACKET("Submit CommandGraph - coroutines version");
      SubmitTiming timing = graph.m_timing;
      timing.id = m_submitIDs++;
      timing.listsCount = 0;
      timing.timeBeforeSubmit.stop();
      timing.submitCpuTime.start();
      auto& nodes = *graph.m_nodes;

      if (!nodes.empty())
      {
        std::unique_ptr<vector<PreparedCommandlist>> lists = std::make_unique<vector<PreparedCommandlist>>();;
        {
          HIGAN_CPU_BRACKET("addNodes");
          timing.addNodes.start();
          *lists = prepareNodes(nodes, false);
          timing.addNodes.stop();
        }

        {
          HIGAN_CPU_BRACKET("GraphSolve");
          timing.graphSolve.start();
          auto firstUsageSeen = checkQueueDependencies(*lists);
          returnResouresToOriginalQueues(*lists, firstUsageSeen);
          handleQueueTransfersWithinRendergraph(*lists, firstUsageSeen);
          timing.graphSolve.stop();
        }

        timing.fillCommandLists.start();

        auto readyLists = makeLiveCommandBuffers(*lists, timing.id);
        // CONSTANTS: I already know here which constant blocks will need copying to gpu. Could make DMA list here.
        timing.listsCount = lists->size();

        std::unique_ptr<css::Task<void>> gcComplete = std::make_unique<css::Task<void>>([&]() -> css::Task<void>
          {
            gc();
            co_return;
          }()
        );

        std::unique_ptr<vector<std::shared_ptr<BarrierSolver>>> solvers = std::make_unique<vector<std::shared_ptr<BarrierSolver>>>();
        solvers->resize(lists->size());

        std::vector<std::shared_ptr<css::Task<std::shared_ptr<css::Task<void>>>>> localPasses;
        css::Task<std::shared_ptr<css::Task<void>>>* prevLocalPass = nullptr;
        

        for (auto&& list : readyLists){
          int offset = list.listIDs[0];
          for (auto id : list.listIDs) {
            localPasses.emplace_back(std::make_shared<css::Task<std::shared_ptr<css::Task<void>>>>(localPass2(prevLocalPass, gcComplete.get(), swapchain, *lists, list, (*solvers)[id], id, offset)));
            prevLocalPass = localPasses.back().get();
          }
        }
        co_await (*prevLocalPass);
        co_await (*(prevLocalPass->get().get()));

        // CONSTANTS: here the constants have been copied to cpu binned memory, ready for copying
        // could launch dma here, before actual lists.

        timing.fillCommandLists.stop();

        timing.submitSolve.start();
        // submit can be "multithreaded" also in the order everything finished, but not in current shape where readyLists is modified.
        //for (auto&& list : lists)

        HIGAN_CPU_BRACKET("Submit Lists");
        while(!readyLists.empty())
        {
          LiveCommandBuffer2 buffer = std::move(readyLists.front());
          readyLists.pop_front();
          auto& vdev = m_devices[buffer.deviceID];
          switch (buffer.queue)
          {
          case QueueType::Dma:
            vdev.m_dmaBuffers.emplace_back(buffer);
            break;
          case QueueType::Compute:
            vdev.m_computeBuffers.emplace_back(buffer);
            break;
          case QueueType::Graphics:
          default:
            vdev.m_gfxBuffers.emplace_back(buffer);
          }
        }
        timing.submitSolve.stop();
      }
      for (auto&& list : nodes) {
        m_commandBuffers.free(std::move(list.list->list));
      }
      timing.submitCpuTime.stop();
      timeOnFlightSubmits.push_back(timing);
      if (graph.m_sequence != InvalidSeqNum)
      {
        m_seqNumRequirements.emplace_back(m_seqTracker.lastSequence());
      }
      co_return;
    }
  }
}