#pragma once
#include "higanbana/graphics/dx12/dx12resources.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/util/pipeline_stream_builder.hpp"
#include "higanbana/graphics/dx12/dx12commandlist.hpp"
#include "higanbana/graphics/common/command_packets.hpp"

#include <optional>

namespace higanbana
{
  namespace backend
  {
    class DX12Device : public prototypes::DeviceImpl
    {
    private:
      GpuInfo m_info;
      bool m_debugLayer;
      ComPtr<ID3D12Device2> m_device;
      ComPtr<ID3D12Device5> m_dxrdevice;
      ComPtr<IDXGIDebug1> m_debug;
      ComPtr<IDXGIFactory4> m_factory;

      FileSystem& m_fs;
      ShaderStorage m_shaders;

      UINT m_nodeMask;
      ComPtr<ID3D12CommandQueue> m_graphicsQueue;
      ComPtr<ID3D12CommandQueue> m_dmaQueue;
      ComPtr<ID3D12CommandQueue> m_computeQueue;
      DX12Fence m_deviceFence;

      StagingDescriptorHeap m_generics;
      //StagingDescriptorHeap m_samplers;
      StagingDescriptorHeap m_rtvs;
      StagingDescriptorHeap m_dsvs;

      Rabbitpool2<DX12ReadbackHeap> m_readbackPool; // make this a new kind of pool, which keeps alive recently used.
      Rabbitpool2<DX12QueryHeap> m_queryHeapPool;
      Rabbitpool2<DX12QueryHeap> m_computeQueryHeapPool;
      Rabbitpool2<DX12QueryHeap> m_dmaQueryHeapPool;
      Rabbitpool2<DX12CommandBuffer> m_copyListPool;
      Rabbitpool2<DX12CommandBuffer> m_computeListPool;
      Rabbitpool2<DX12CommandBuffer> m_graphicsListPool;
      Rabbitpool2<DX12Fence> m_fencePool;
      Rabbitpool2<DX12Semaphore> m_semaPool;

      std::shared_ptr<DX12UploadHeap> m_constantsUpload;
      std::shared_ptr<DX12UploadHeap> m_dynamicUpload;

      std::shared_ptr<DX12DynamicDescriptorHeap> m_dynamicGpuDescriptors;

      DX12CPUDescriptor m_nullBufferUAV;
      DX12CPUDescriptor m_nullBufferSRV;
      DX12CPUDescriptor m_nullTextureUAV;
      DX12CPUDescriptor m_nullTextureSRV;

      // new stuff
      DX12Resources m_allRes;

      // restrictions&info
      struct FeatureData
      {
        D3D12_FEATURE_DATA_D3D12_OPTIONS  opt0 = {}; 
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 opt1 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS2 opt2 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 opt3 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS4 opt4 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 opt5 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 opt6 = {};
      } m_features;

      // locks and stuff
      std::mutex m_deviceMutex;

      // shader debug gpu buffer
      DX12Buffer m_shaderDebugBuffer;
      DX12GPUDescriptor m_shaderDebugTable;
      DX12CPUDescriptor m_shaderDebugTableCPU;

      // timings
      int64_t m_graphicsTimeOffset;
      int64_t m_computeTimeOffset;
      int64_t m_dmaTimeOffset;

      friend class DX12CommandList;

      DX12PipelineStateStreamBuilder getDescStream(GraphicsPipelineDescriptor::Desc& d, gfxpacket::RenderPassBegin& subpass);
    public:
      DX12Device(GpuInfo info, ComPtr<ID3D12Device2> device, ComPtr<IDXGIFactory4> factory, FileSystem& fs, bool debugLayer);
      ~DX12Device();

      std::lock_guard<std::mutex> deviceLock() { return std::lock_guard<std::mutex>(m_deviceMutex);}

      DeviceStatistics statsOfResourcesInUse() override;

      DX12Resources& allResources();

      D3D12_RESOURCE_DESC fillPlacedBufferInfo(ResourceDescriptor descriptor);
      D3D12_RESOURCE_DESC fillPlacedTextureInfo(ResourceDescriptor descriptor);

      //void updatePipeline(GraphicsPipeline& pipeline, gfxpacket::RenderpassBegin& subpass);
      //void updatePipeline(ComputePipeline& pipeline);

      std::optional<DX12OldPipeline> updatePipeline(ResourceHandle pipeline, gfxpacket::RenderPassBegin& renderpass);
      std::optional<DX12OldPipeline> updatePipeline(ResourceHandle pipeline);

      // impl
      void createPipeline(ResourceHandle handle, GraphicsPipelineDescriptor layout) override;
      void createPipeline(ResourceHandle handle, ComputePipelineDescriptor layout) override;

      std::shared_ptr<prototypes::SwapchainImpl> createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor) override;
      void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, SwapchainDescriptor descriptor) override;
      void ensureSwapchainColorspace(std::shared_ptr<DX12Swapchain> sc, SwapchainDescriptor& descriptor);
      int fetchSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc, vector<ResourceHandle>& handles) override;
      int tryAcquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;
      int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;

      void releaseHandle(ResourceHandle handle) override;
      void releaseViewHandle(ViewResourceHandle handle) override;
      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      void createRenderpass(ResourceHandle handle) override;

      void createHeap(ResourceHandle handle, HeapDescriptor desc) override;

      void createBuffer(ResourceHandle handle, ResourceDescriptor& desc) override;
      void createBuffer(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) override;
      void createBufferView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void createTexture(ResourceHandle handle, ResourceDescriptor& desc) override;
      void createTexture(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) override;
      void createTextureView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void createShaderArgumentsLayout(ResourceHandle handle, ShaderArgumentsLayoutDescriptor& desc) override;
      void createShaderArguments(ResourceHandle handle, ShaderArgumentsDescriptor& binding) override;

      std::shared_ptr<backend::TimelineSemaphoreImpl> createSharedSemaphore() override;

      std::shared_ptr<backend::SharedHandle> openSharedHandle(std::shared_ptr<backend::TimelineSemaphoreImpl>) override;
      std::shared_ptr<backend::SharedHandle> openSharedHandle(HeapAllocation allocation) override;
      std::shared_ptr<backend::SharedHandle> openSharedHandle(ResourceHandle resource) override;
      std::shared_ptr<backend::SharedHandle> openForInteropt(ResourceHandle resource) override;
      std::shared_ptr<backend::TimelineSemaphoreImpl> createSemaphoreFromHandle(std::shared_ptr<backend::SharedHandle>) override;
      void createHeapFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared) override;
      void createBufferFromHandle(ResourceHandle handle, std::shared_ptr<backend::SharedHandle> shared, HeapAllocation heapAllocation, ResourceDescriptor& desc) override;
      void createTextureFromHandle(ResourceHandle handle, std::shared_ptr<backend::SharedHandle> shared, ResourceDescriptor& desc) override;

      size_t availableDynamicMemory() override;
      void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, FormatType format) override;
      void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned stride) override;
      void dynamicImage(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned rowPitch) override;

      void readbackBuffer(ResourceHandle readback, size_t bytes) override;
      MemView<uint8_t> mapReadback(ResourceHandle readback) override;
      void unmapReadback(ResourceHandle readback) override;

      // commandlist things and gpu-cpu/gpu-gpu synchronization primitives
      DX12QueryHeap createGraphicsQueryHeap(unsigned counters);
      DX12QueryHeap createComputeQueryHeap(unsigned counters);
      DX12QueryHeap createDMAQueryHeap(unsigned counters);

      DX12ReadbackHeap createReadback(unsigned pages, unsigned pageSize);
      DX12CommandBuffer createList(D3D12_COMMAND_LIST_TYPE type);
      DX12Fence         createNativeFence();
      DX12Semaphore createNativeSemaphore();
      std::shared_ptr<CommandBufferImpl> createDMAList() override;
      std::shared_ptr<CommandBufferImpl> createComputeList() override;
      std::shared_ptr<CommandBufferImpl> createGraphicsList() override;
      std::shared_ptr<SemaphoreImpl>     createSemaphore() override;
      std::shared_ptr<FenceImpl>         createFence() override;
      std::shared_ptr<TimelineSemaphoreImpl>     createTimelineSemaphore() override;

      void submit(
        ComPtr<ID3D12CommandQueue> queue,
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<TimelineSemaphoreInfo> waitTimelines,
        MemView<TimelineSemaphoreInfo> signaltimelines,
        std::optional<std::shared_ptr<FenceImpl>>         fence);

      void submitDMA(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<TimelineSemaphoreInfo> waitTimelines,
        MemView<TimelineSemaphoreInfo> signaltimelines,
        std::optional<std::shared_ptr<FenceImpl>>         fence) override;

      void submitCompute(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<TimelineSemaphoreInfo> waitTimelines,
        MemView<TimelineSemaphoreInfo> signaltimelines,
        std::optional<std::shared_ptr<FenceImpl>>         fence) override;

      void submitGraphics(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<TimelineSemaphoreInfo> waitTimelines,
        MemView<TimelineSemaphoreInfo> signaltimelines,
        std::optional<std::shared_ptr<FenceImpl>>         fence) override;

      void waitFence(std::shared_ptr<FenceImpl>     fence) override;
      bool checkFence(std::shared_ptr<FenceImpl>    fence) override;
      uint64_t completedValue(std::shared_ptr<backend::TimelineSemaphoreImpl> tlSema) override;
      void waitTimeline(std::shared_ptr<backend::TimelineSemaphoreImpl> tlSema, uint64_t value) override;
      void present(std::shared_ptr<prototypes::SwapchainImpl> swapchain, std::shared_ptr<SemaphoreImpl> renderingFinished, int index) override;
    };
  }
}
#endif