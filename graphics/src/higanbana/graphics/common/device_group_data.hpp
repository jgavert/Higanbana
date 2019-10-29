#pragma once

#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/desc/formats.hpp"
#include "higanbana/graphics/desc/resource_state.hpp"
#include "higanbana/graphics/common/barrier_solver.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/commandgraph.hpp"
#include "higanbana/graphics/desc/timing.hpp"

#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <optional>
#include <functional>
#include <mutex>

namespace higanbana
{
  namespace backend
  {
    class DelayedRelease
    {
      struct Garbage
      {
        SeqNum sequence;
        vector<ResourceHandle> trash;
        vector<ViewResourceHandle> viewTrash;
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

      void insert(SeqNum sequence, ViewResourceHandle handle)
      {
        std::lock_guard<std::mutex> guard(lock);
        ensureGarbage(sequence);

        auto& bck = m_garbage.back();
        bck.viewTrash.push_back(handle);
      }

      Garbage garbageCollection(SeqNum completedTo)
      {
        std::lock_guard<std::mutex> guard(lock);
        Garbage garb;
        while(!m_garbage.empty() && m_garbage.front().sequence <= completedTo)
        {
          auto& newgarb = m_garbage.front();
          garb.trash.insert(garb.trash.end(), newgarb.trash.begin(), newgarb.trash.end());
          garb.viewTrash.insert(garb.viewTrash.end(), newgarb.viewTrash.begin(), newgarb.viewTrash.end());
          m_garbage.pop_front();
        }
        return garb;
      }
    };
    struct LiveCommandBuffer2
    {
      int deviceID;
      QueueType queue;
      SeqNum started;
      vector<std::shared_ptr<backend::SemaphoreImpl>> wait;
      vector<std::shared_ptr<backend::CommandBufferImpl>> lists;
      vector<std::shared_ptr<backend::SemaphoreImpl>> signal;
      std::shared_ptr<backend::FenceImpl> fence;
      vector<ReadbackPromise> readbacks;
      // timings
      uint64_t submitID;
      CommandListTiming listTiming;
    };

    struct QueueTransfer 
    {
      ResourceType type;
      int id;
      QueueType fromOrTo;
    };
    
    struct PreparedCommandlist
    {
      int device = 0;
      QueueType type;
      vector<backend::CommandBuffer> buffers;
      vector<QueueTransfer> acquire;
      vector<QueueTransfer> release;
      DynamicBitfield requirementsBuf;
      DynamicBitfield requirementsTex;
      bool waitGraphics = false, waitCompute = false, waitDMA = false;
      bool signal = false; // signal own queue sema
      std::shared_ptr<SemaphoreImpl> acquireSema;
      bool presents = false;
      bool isLastList = false;
      vector<ReadbackPromise> readbacks;
      CommandListTiming timing;
    };

    struct FirstUseResource
    {
      int deviceID;
      ResourceType type;
      int id;
      QueueType queue;
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
        HandleVector<ResourceState> m_bufferStates;
        HandleVector<TextureResourceState> m_textureStates;
        HandleVector<vector<ViewResourceHandle>> shaderArguments;
        QueueStates qStates;
        std::shared_ptr<SemaphoreImpl> graphicsQSema;
        std::shared_ptr<SemaphoreImpl> computeQSema;
        std::shared_ptr<SemaphoreImpl> dmaQSema;
        deque<LiveCommandBuffer2> m_gfxBuffers;
        deque<LiveCommandBuffer2> m_computeBuffers;
        deque<LiveCommandBuffer2> m_dmaBuffers;
      };
      vector<VirtualDevice> m_devices;
      CommandBufferPool m_commandBuffers;
      SequenceTracker m_seqTracker; // used to track only commandlists
      std::unique_ptr<DelayedRelease> m_delayer;
      HandleManager m_handles;

      // used to free resources
      deque<SeqNum> m_seqNumRequirements;
      SeqNum m_currentSeqNum = 0;
      SeqNum m_completedLists = 0;

      // timings
      uint64_t m_submitIDs = 0;
      deque<SubmitTiming> timeOnFlightSubmits;
      deque<SubmitTiming> timeSubmitsFinished;

      DeviceGroupData(vector<std::shared_ptr<prototypes::DeviceImpl>> impl, vector<GpuInfo> infos);
      DeviceGroupData(DeviceGroupData&& data) = default;
      DeviceGroupData(const DeviceGroupData& data) = delete;
      DeviceGroupData& operator=(DeviceGroupData&& data) = default;
      DeviceGroupData& operator=(const DeviceGroupData& data) = delete;
      ~DeviceGroupData();

      void checkCompletedLists();
      void gc();
      void garbageCollection();
      void waitGpuIdle();

      // helper
      void configureBackbufferViews(Swapchain& sc);
      std::shared_ptr<ResourceHandle> sharedHandle(ResourceHandle handle);
      std::shared_ptr<ViewResourceHandle> sharedViewHandle(ViewResourceHandle handle);

      Swapchain createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor);
      void adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor);
      std::optional<TextureRTV> acquirePresentableImage(Swapchain& swapchain);
      TextureRTV* tryAcquirePresentableImage(Swapchain& swapchain);

      Renderpass createRenderpass();
      ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);
      GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);

      Buffer createBuffer(ResourceDescriptor desc);
      Texture createTexture(ResourceDescriptor desc);

      BufferIBV createBufferIBV(Buffer texture, ShaderViewDescriptor viewDesc);
      BufferSRV createBufferSRV(Buffer texture, ShaderViewDescriptor viewDesc);
      BufferUAV createBufferUAV(Buffer texture, ShaderViewDescriptor viewDesc);
      TextureSRV createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureUAV createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureRTV createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc);
      TextureDSV createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc);

      DynamicBufferView dynamicBuffer(MemView<uint8_t> view, FormatType type);
      DynamicBufferView dynamicBuffer(MemView<uint8_t> view, unsigned stride);
      DynamicBufferView dynamicImage(MemView<uint8_t> range, unsigned stride);

      // ShaderArguments
      ShaderArgumentsLayout createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor desc);
      ShaderArguments createShaderArguments(ShaderArgumentsDescriptor& binding);

      // streaming
      bool uploadInitialTexture(Texture& tex, CpuImage& image);

      // commandgraph
      CommandGraph startCommandGraph();
      void submit(std::optional<Swapchain> swapchain, CommandGraph& graph);
      void present(Swapchain& swapchain);

      // test
      void generateReadbackCommands(VirtualDevice& vdev, MemView<CommandBuffer>& buffers, QueueType queue, vector<ReadbackPromise>& readbacks);
      void fillCommandBuffer(std::shared_ptr<CommandBufferImpl> nativeList, VirtualDevice& vdev, MemView<CommandBuffer>& buffer, QueueType queue, vector<QueueTransfer>& acquire, vector<QueueTransfer>& release, CommandListTiming& timing);
      vector<FirstUseResource> checkQueueDependencies(vector<PreparedCommandlist>& lists);
    };
  }
}