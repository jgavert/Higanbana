#include "graphics/common/device_group_data.hpp"
#include "graphics/common/gpudevice.hpp"
#include "graphics/common/graphicssurface.hpp"
#include "graphics/common/implementation.hpp"
#include "graphics/common/semaphore.hpp"
#include "graphics/common/cpuimage.hpp"

#include "core/math/utils.hpp"

namespace faze
{
  namespace backend
  {
    // device
    DeviceGroupData::DeviceGroupData(vector<std::shared_ptr<prototypes::DeviceImpl>> impls)
      : m_impl(impls)
    {
      // all devices have their own heap managers
      for (int i = 0; i < m_impl.size(); ++i)
      {
        m_heaps.push_back(HeapManager());
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
          m_impl[deviceID]->waitFence(m_buffers.front().fence);
        }
        int buffersToFree = 0;
        int buffersWithoutFence = 0;
        int bufferCount = static_cast<int>(m_buffers.size());
        for (int i = 0; i < bufferCount; ++i)
        {
          if (m_buffers[i].fence)
          {
            if (!m_impl[m_buffers[i].deviceID]->checkFence(m_buffers[i].fence))
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
      for (auto&& impl : m_impl)
      {
        if (impl)
        {
          impl->waitGpuIdle();
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
            m_impl[liveBuffer.deviceID]->waitFence(liveBuffer.fence);
          }
        }

        while (!m_buffers.empty())
        {
          m_buffers.pop_front();
        }
      }
    }

    constexpr const int SwapchainDeviceID = 0;

    Swapchain DeviceGroupData::createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor)
    {
      // assume that first device is the one in charge of presenting. TODO: swapchain is created always on device 0
      auto sc = m_impl[SwapchainDeviceID]->createSwapchain(surface, descriptor);
      auto swapchain = Swapchain(sc);
      // get backbuffers to swapchain
      auto buffers = m_impl[SwapchainDeviceID]->getSwapchainTextures(swapchain.impl());
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

    void DeviceGroupData::adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor)
    {
      // stop gpu, possibly wait for last 'present' by inserting only fence to queue.
      auto fenceForSwapchain = m_impl[SwapchainDeviceID]->createFence();
      // assuming that only graphics queue accesses swapchain resources.
      m_impl[SwapchainDeviceID]->submitGraphics({}, {}, {}, fenceForSwapchain);
      m_impl[SwapchainDeviceID]->waitFence(fenceForSwapchain);
      waitGpuIdle();
      // wait all idle work.
      // release current swapchain backbuffers
      swapchain.setBackbuffers({}); // technically this frees the textures if they are not used anywhere.
      // go collect the trash.
      gc();

      // blim
      m_impl[SwapchainDeviceID]->adjustSwapchain(swapchain.impl(), descriptor);
      // get new backbuffers... seems like we do it here.
      auto buffers = m_impl[SwapchainDeviceID]->getSwapchainTextures(swapchain.impl());
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

    TextureRTV DeviceGroupData::acquirePresentableImage(Swapchain& swapchain)
    {
      int index = m_impl[SwapchainDeviceID]->acquirePresentableImage(swapchain.impl());
      return swapchain.buffers()[index];
    }

    TextureRTV* DeviceGroupData::tryAcquirePresentableImage(Swapchain& swapchain)
    {
      int index = m_impl[SwapchainDeviceID]->tryAcquirePresentableImage(swapchain.impl());
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
      F_ASSERT(false, "unimplemented");
      return ComputePipeline();
      //return ComputePipeline(m_impl->createPipeline(desc), desc);
    }

    GraphicsPipeline DeviceGroupData::createGraphicsPipeline(GraphicsPipelineDescriptor desc)
    {
      return GraphicsPipeline(desc);
    }

    Buffer DeviceGroupData::createBuffer(ResourceDescriptor desc)
    {
      desc = desc.setDimension(FormatDimension::Buffer);
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createBuffer(allo, desc);
      auto tracker = m_trackerbuffers->makeTracker(newId(), allo.allocation);
      return Buffer(data, tracker, std::make_shared<ResourceDescriptor>(std::move(desc)));
    }

    SharedBuffer DeviceGroupData::createSharedBuffer(DeviceData & secondary, ResourceDescriptor desc)
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

    Texture DeviceGroupData::createTexture(ResourceDescriptor desc)
    {
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createTexture(allo, desc);
      auto tracker = m_trackertextures->makeTracker(newId(), allo.allocation);
      return Texture(data, tracker, std::make_shared<ResourceDescriptor>(desc));
    }

    SharedTexture DeviceGroupData::createSharedTexture(DeviceData & secondary, ResourceDescriptor desc)
    {
      auto data = m_impl->createTexture(desc);
      auto handle = m_impl->openSharedHandle(data);
      auto secondaryData = secondary.m_impl->createTextureFromHandle(handle, desc);
      return SharedTexture(data, secondaryData, std::make_shared<int64_t>(newId()), std::make_shared<ResourceDescriptor>(std::move(desc)));
    }

    BufferSRV DeviceGroupData::createBufferSRV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createBufferView(buffer.native(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      return BufferSRV(buffer, data);
    }

    BufferUAV DeviceGroupData::createBufferUAV(Buffer buffer, ShaderViewDescriptor viewDesc)
    {
      auto data = m_impl->createBufferView(buffer.native(), buffer.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      return BufferUAV(buffer, data);
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
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadOnly));
      return TextureSRV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), true), viewDesc.m_format);
    }

    TextureUAV DeviceGroupData::createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::ReadWrite));
      return TextureUAV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), false), viewDesc.m_format);
    }

    TextureRTV DeviceGroupData::createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::RenderTarget));
      return TextureRTV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), false), viewDesc.m_format);
    }

    TextureDSV DeviceGroupData::createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc)
    {
      if (viewDesc.m_format == FormatType::Unknown)
      {
        viewDesc.m_format = texture.desc().desc.format;
      }
      auto data = m_impl->createTextureView(texture.native(), texture.desc(), viewDesc.setType(ResourceShaderType::DepthStencil));
      return TextureDSV(texture, data, newId(), rangeFrom(viewDesc, texture.desc(), false), viewDesc.m_format);
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> view, FormatType type)
    {
      return DynamicBufferView(nullptr);
    }

    DynamicBufferView DeviceGroupData::dynamicBuffer(MemView<uint8_t> view, unsigned stride)
    {
      return DynamicBufferView(nullptr);
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