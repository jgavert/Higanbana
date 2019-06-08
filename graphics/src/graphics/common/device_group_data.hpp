#pragma once

#include <graphics/common/resources.hpp>
#include <graphics/common/handle.hpp>
#include <graphics/desc/formats.hpp>
#include <core/system/memview.hpp>
#include <core/system/SequenceTracker.hpp>

#include <functional>
#include <mutex>

namespace faze
{
  namespace backend
  {
    class DelayedRelease
    {
      struct Garbage
      {
        SeqNum sequence;
        vector<ResourceHandle> trash;
      };
      deque<Garbage> m_garbage;
      std::mutex lock; 

      void ensureGarbage(SeqNum sequence)
      {
        if (m_garbage.empty() || m_garbage.back().sequence != sequence)
        {
          m_garbage.push_back(Garbage{sequence, {}});
        }
      }
    public:
      DelayedRelease()
      {

      }
      
      void insert(SeqNum sequence, ResourceHandle handle)
      {
        std::lock_guard<std::mutex> guard(lock);
        ensureGarbage(sequence);

        auto& bck = m_garbage.back();
        bck.trash.push_back(handle);
      }

      vector<ResourceHandle> garbageCollection(SeqNum completedTo)
      {
        std::lock_guard<std::mutex> guard(lock);
        vector<ResourceHandle> handles;
        while(!m_garbage.empty() && m_garbage.front().sequence <= completedTo)
        {
          auto& newgarb = m_garbage.front().trash;
          handles.insert(handles.end(), newgarb.begin(), newgarb.end());
          m_garbage.pop_front();
        }
        return handles;
      }
    };

    struct DeviceGroupData : std::enable_shared_from_this<DeviceGroupData>
    {
      // consist of device specific/grouped classes
      struct VirtualDevice
      {
        int id;
        std::shared_ptr<prototypes::DeviceImpl> device;
        HeapManager heaps;
        GpuInfo info;
        HandleVector<GpuHeapAllocation> m_buffers;
        HandleVector<GpuHeapAllocation> m_textures;
      };
      vector<VirtualDevice> m_devices;

      SequenceTracker m_seqTracker;
      DelayedRelease m_delayer;
      HandleManager m_handles;
      deque<LiveCommandBuffer2> m_buffers;

      DeviceGroupData(vector<std::shared_ptr<prototypes::DeviceImpl>> impl, vector<GpuInfo> infos);
      DeviceGroupData(DeviceGroupData&& data) = default;
      DeviceGroupData(const DeviceGroupData& data) = delete;
      DeviceGroupData& operator=(DeviceGroupData&& data) = default;
      DeviceGroupData& operator=(const DeviceGroupData& data) = delete;
      ~DeviceGroupData();

      CommandGraph startCommandGraph();
      void checkCompletedLists();
      void gc();
      void garbageCollection();
      void waitGpuIdle();

      // helper
      void configureBackbufferViews(Swapchain& sc);
      std::shared_ptr<ResourceHandle> sharedHandle(ResourceHandle handle);

      Swapchain createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor);
      void adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor);
      TextureRTV acquirePresentableImage(Swapchain& swapchain);
      TextureRTV* tryAcquirePresentableImage(Swapchain& swapchain);

      Renderpass createRenderpass();
      ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);
      GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);

      Buffer createBuffer(ResourceDescriptor desc);
      Texture createTexture(ResourceDescriptor desc);

      BufferSRV createBufferSRV(Buffer texture, ShaderViewDescriptor viewDesc);
      BufferUAV createBufferUAV(Buffer texture, ShaderViewDescriptor viewDesc);
      TextureSRV createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureUAV createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureRTV createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureDSV createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc);

      DynamicBufferView dynamicBuffer(MemView<uint8_t> view, FormatType type);
      DynamicBufferView dynamicBuffer(MemView<uint8_t> view, unsigned stride);

      // streaming
      bool uploadInitialTexture(Texture& tex, CpuImage& image);

      // commandgraph
      void submit(Swapchain& swapchain, CommandGraph graph);
      void submit(CommandGraph graph);
      void explicitSubmit(CommandGraph graph);
      void present(Swapchain& swapchain);
    };
  }
}