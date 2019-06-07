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
        m_devices.push_back({impls[i], HeapManager(), infos[i]});
      }
    }

    void DeviceGroupData::checkCompletedLists()
    {
      if (!m_buffers.empty())
      {
        if (m_buffers.size() > 8) // throttle so that we don't go too fast.
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

        while (buffersToFree > 0)
        {
          m_buffers.pop_front();
          buffersToFree--;
        }
      }
    }

    void DeviceGroupData::gc()
    {
      checkCompletedLists();

/*
      for (auto&& allocation : m_trackerbuffers->getAllocations())
      {
        m_heaps.release(allocation);
      }

      for (auto&& allocation : m_trackertextures->getAllocations())
      {
        m_heaps.release(allocation);
      }

      m_heaps.emptyHeaps();
      */

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
        backbuffers[i] = createTextureRTV(tex, ShaderViewDescriptor());
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
      F_ASSERT(false, "unimplemented");
      return Renderpass();
      //return Renderpass(m_impl->createRenderpass());
    }

    ComputePipeline DeviceGroupData::createComputePipeline(ComputePipelineDescriptor desc)
    {
      auto handle = m_handles.allocate(ResourceType::Pipeline);
      for(auto&& device : m_devices)
      {
        device.device->createPipeline(handle, desc);
      }
      return ComputePipeline(sharedHandle(handle), desc);
      //return ComputePipeline(m_impl->createPipeline(desc), desc);
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
        auto allo = vdev.heaps.allocate(vdev.device.get(), memRes); // get heap corresponding to requirements
        vdev.device->createBuffer(handle, allo, desc); // assign and create buffer
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
        auto allo = vdev.heaps.allocate(vdev.device.get(), memRes); // get heap corresponding to requirements
        vdev.device->createTexture(handle, allo, desc); // assign and create buffer
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

    SubresourceRange rangeFrom(const ShaderViewDescriptor & view, const ResourceDescriptor & desc, bool srv)
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

    bool DeviceGroupData::uploadInitialTexture(Texture & tex, CpuImage & image)
    {

      return true;
    }

    void DeviceGroupData::submit(Swapchain & swapchain, CommandGraph graph)
    {
    }

    void DeviceGroupData::submit(CommandGraph graph)
    {
    }

    void DeviceGroupData::explicitSubmit(CommandGraph graph)
    {

    }

    void DeviceGroupData::garbageCollection()
    {
      //m_impl->collectTrash();
    }

    void DeviceGroupData::present(Swapchain & swapchain)
    {
      //m_impl->present(swapchain.impl(), swapchain.impl()->renderSemaphore());
    }
  }
}