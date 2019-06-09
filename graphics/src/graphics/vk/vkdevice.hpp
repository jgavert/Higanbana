#pragma once
#include <graphics/vk/vkresources.hpp>
namespace faze
{
  namespace backend
  {
    class VulkanDevice : public prototypes::DeviceImpl
    {
    private:
      vk::Device                  m_device;
      vk::PhysicalDevice		      m_physDevice;
      bool                        m_debugLayer;
      std::vector<vk::QueueFamilyProperties> m_queues;
      bool                        m_singleQueue;
      bool                        m_computeQueues;
      bool                        m_dmaQueues;
      bool                        m_graphicQueues;
      GpuInfo                     m_info;
      ShaderStorage               m_shaders;
      int64_t                     m_resourceID = 1;

      int                         m_mainQueueIndex;
      int                         m_computeQueueIndex = -1;
      int                         m_copyQueueIndex = -1;
      vk::Queue                   m_mainQueue;
      vk::Queue                   m_computeQueue;
      vk::Queue                   m_copyQueue;

      vk::Sampler                 m_bilinearSampler;
      vk::Sampler                 m_pointSampler;
      vk::Sampler                 m_bilinearSamplerWrap;
      vk::Sampler                 m_pointSamplerWrap;

      Rabbitpool2<VulkanSemaphore>    m_semaphores;
      Rabbitpool2<VulkanFence>        m_fences;
      Rabbitpool2<VulkanCommandList>  m_copyListPool;
      Rabbitpool2<VulkanCommandList>  m_computeListPool;
      Rabbitpool2<VulkanCommandList>  m_graphicsListPool;

      unordered_map<size_t, std::shared_ptr<vk::RenderPass>> m_renderpasses;

      struct FreeQueues
      {
        int universalIndex;
        int graphicsIndex;
        int computeIndex;
        int dmaIndex;
        std::vector<uint32_t> universal;
        std::vector<uint32_t> graphics;
        std::vector<uint32_t> compute;
        std::vector<uint32_t> dma;
      } m_freeQueueIndexes;

      // new stuff

      std::shared_ptr<SequenceTracker> m_seqTracker;
      std::shared_ptr<VulkanUploadHeap> m_dynamicUpload;

      struct Garbage
      {
        vector<VkUploadBlock> dynamicBuffers;
        vector<vk::Image> textures;
        vector<vk::Buffer> buffers;
        vector<vk::ImageView> textureviews;
        vector<vk::BufferView> bufferviews;
        vector<vk::Pipeline> pipelines;
        vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        vector<vk::PipelineLayout>      pipelineLayouts;
        vector<vk::DeviceMemory> heaps;
      };

      std::shared_ptr<Garbage> m_trash;
      deque<std::pair<SeqNum, Garbage>> m_collectableTrash;

      // new new stuff
      // HandleVector<VulkanTexture>
      struct Resources
      {
        HandleVector<VulkanTexture> tex;
        HandleVector<VulkanBuffer> buf;
        HandleVector<VulkanBufferView> bufSRV;
        HandleVector<VulkanBufferView> bufUAV;
        HandleVector<VulkanBufferView> bufIBV;
        HandleVector<VulkanTextureView> texSRV;
        HandleVector<VulkanTextureView> texUAV;
        HandleVector<VulkanTextureView> texRTV;
        HandleVector<VulkanTextureView> texDSV;
        HandleVector<VulkanPipeline> pipelines;
        HandleVector<VulkanRenderpass> renderpasses;
        HandleVector<VulkanHeap> heaps;
      } m_allRes;
    public:
      VulkanDevice(
        vk::Device device,
        vk::PhysicalDevice physDev,
        FileSystem& fs,
        std::vector<vk::QueueFamilyProperties> queues,
        GpuInfo info,
        bool debugLayer);
      ~VulkanDevice();

      vk::Device native() { return m_device; }

      std::shared_ptr<vk::RenderPass> createRenderpass(const vk::RenderPassCreateInfo& info);

      //void updatePipeline(GraphicsPipeline& pipeline, vk::RenderPass rp, gfxpacket::RenderpassBegin& subpass);
      void updatePipeline(ComputePipeline& pipeline);

      // implementation
      std::shared_ptr<prototypes::SwapchainImpl> createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor) override;
      void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, SwapchainDescriptor descriptor) override;
      int fetchSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc, vector<ResourceHandle>& handles) override;
      int tryAcquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;
      int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;

      void releaseHandle(ResourceHandle handle) override;
      void collectTrash() override;
      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      void createRenderpass(ResourceHandle handle) override;
      vector<vk::DescriptorSetLayoutBinding> gatherSetLayoutBindings(ShaderInputDescriptor desc, vk::ShaderStageFlags flags);
      void createPipeline(ResourceHandle handle, GraphicsPipelineDescriptor layout) override;
      void createPipeline(ResourceHandle handle, ComputePipelineDescriptor layout) override;

      void createHeap(ResourceHandle handle, HeapDescriptor desc) override;

      void createBuffer(ResourceHandle handle, ResourceDescriptor& desc) override;
      void createBuffer(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) override;
      void createBufferView(ResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void createTexture(ResourceHandle handle, ResourceDescriptor& desc) override;
      void createTexture(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) override;
      void createTextureView(ResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;

      std::shared_ptr<SemaphoreImpl> createSharedSemaphore() override { return nullptr; };

      std::shared_ptr<SharedHandle> openSharedHandle(std::shared_ptr<SemaphoreImpl>) override { return nullptr; };
      std::shared_ptr<SharedHandle> openSharedHandle(HeapAllocation) override { return nullptr; };
      std::shared_ptr<SharedHandle> openSharedHandle(ResourceHandle handle) override { return nullptr; };
      std::shared_ptr<SemaphoreImpl> createSemaphoreFromHandle(std::shared_ptr<SharedHandle>) override { return nullptr; };
      void createBufferFromHandle(ResourceHandle , std::shared_ptr<SharedHandle>, HeapAllocation, ResourceDescriptor&) override { return; };
      void createTextureFromHandle(ResourceHandle , std::shared_ptr<SharedHandle>, ResourceDescriptor&) override { return; };

      std::shared_ptr<prototypes::DynamicBufferViewImpl> dynamic(MemView<uint8_t> bytes, FormatType format) override;
      std::shared_ptr<prototypes::DynamicBufferViewImpl> dynamic(MemView<uint8_t> bytes, unsigned stride) override;
      std::shared_ptr<prototypes::DynamicBufferViewImpl> dynamicImage(MemView<uint8_t> bytes, unsigned rowPitch) override;

      // commandlist stuff
      VulkanCommandList createCommandBuffer(int queueIndex);
      void resetListNative(VulkanCommandList list);
      std::shared_ptr<CommandBufferImpl> createDMAList() override;
      std::shared_ptr<CommandBufferImpl> createComputeList() override;
      std::shared_ptr<CommandBufferImpl> createGraphicsList() override;
      std::shared_ptr<SemaphoreImpl>     createSemaphore() override;
      std::shared_ptr<FenceImpl>         createFence() override;

      void submitToQueue(
        vk::Queue queue,
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence);

      void submitDMA(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence) override;

      void submitCompute(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence) override;

      void submitGraphics(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        MemView<std::shared_ptr<FenceImpl>>         fence) override;

      void waitFence(std::shared_ptr<FenceImpl>     fence) override;
      bool checkFence(std::shared_ptr<FenceImpl>    fence) override;

      void present(std::shared_ptr<prototypes::SwapchainImpl> swapchain, std::shared_ptr<SemaphoreImpl> renderingFinished) override;
    };
  }
}