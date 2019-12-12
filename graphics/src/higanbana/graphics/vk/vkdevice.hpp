#pragma once
#include "higanbana/graphics/vk/vkresources.hpp"
#include <optional>
#include <mutex>

namespace higanbana
{
  namespace backend
  {
    class VulkanDevice : public prototypes::DeviceImpl
    {
    private:
      vk::Device                  m_device;
      vk::PhysicalDevice          m_physDevice;
      vk::PhysicalDeviceLimits    m_limits;
      vk::DispatchLoaderDynamic   m_dynamicDispatch;
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

      StaticSamplers              m_samplers;

      std::shared_ptr<VulkanDescriptorPool> m_descriptors;

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
      std::shared_ptr<VulkanConstantUploadHeap> m_constantAllocators;

      // descriptor stuff
      VulkanShaderArgumentsLayout m_defaultDescriptorLayout;
      // shader debug
      VulkanBuffer m_shaderDebugBuffer;

      // descriptor stats
      size_t m_descriptorSetsInUse;
      size_t m_maxDescriptorSets;

      Resources m_allRes;

      // thread lock stuff
      std::mutex m_deviceLock;
    public:
      VulkanDevice(
        vk::Device device,
        vk::PhysicalDevice physDev,
        vk::DispatchLoaderDynamic dynamicDispatch,
        FileSystem& fs,
        std::vector<vk::QueueFamilyProperties> queues,
        GpuInfo info,
        bool debugLayer);
      ~VulkanDevice();

      vk::Device native() { return m_device; }

      template<typename VulkanObject>
      void setDebugUtilsObjectNameEXT(VulkanObject obj, const char* name)
      {
        if (m_dynamicDispatch.vkSetDebugUtilsObjectNameEXT)
        {
          m_device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT()
          .setObjectType(VulkanObject::objectType)
          .setObjectHandle((uint64_t)(static_cast<typename VulkanObject::CType>(obj)))
          .setPObjectName(name)
            , m_dynamicDispatch);
        }
      }

      vk::DescriptorSetLayout defaultDescLayout() { return m_defaultDescriptorLayout.native(); }

      vk::DispatchLoaderDynamic dispatcher() { return m_dynamicDispatch; }
      bool debugDevice() { return m_debugLayer; }

      DeviceStatistics statsOfResourcesInUse() override;
      Resources& allResources() { return m_allRes; }
      std::lock_guard<std::mutex> deviceLock() { return std::lock_guard<std::mutex>(m_deviceLock); }

      QueueIndexes queueIndexes() const { return QueueIndexes{m_mainQueueIndex, m_computeQueueIndex, m_copyQueueIndex}; }

      vk::RenderPass createRenderpass(const vk::RenderPassCreateInfo& info);

      std::optional<vk::Pipeline> updatePipeline(ResourceHandle pipeline, gfxpacket::RenderPassBegin& renderpass);
      std::optional<vk::Pipeline> updatePipeline(ResourceHandle pipeline);

      // implementation
      std::shared_ptr<prototypes::SwapchainImpl> createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor) override;
      void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, SwapchainDescriptor descriptor) override;
      int fetchSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc, vector<ResourceHandle>& handles) override;
      int tryAcquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;
      int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;

      void releaseHandle(ResourceHandle handle) override;
      void releaseViewHandle(ViewResourceHandle handle) override;
      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      void createRenderpass(ResourceHandle handle) override;
      vector<vk::DescriptorSetLayoutBinding> defaultSetLayoutBindings(vk::ShaderStageFlags flags);
      vector<vk::DescriptorSetLayoutBinding> gatherSetLayoutBindings(ShaderArgumentsLayoutDescriptor desc, vk::ShaderStageFlags flags);
      void createPipeline(ResourceHandle handle, GraphicsPipelineDescriptor layout) override;
      void createPipeline(ResourceHandle handle, ComputePipelineDescriptor layout) override;

      void createHeap(ResourceHandle handle, HeapDescriptor desc) override;

      void createBuffer(ResourceHandle handle, ResourceDescriptor& desc) override;
      void createBuffer(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) override;
      void createBufferView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void createTexture(ResourceHandle handle, ResourceDescriptor& desc) override;
      void createTexture(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) override;
      void createTextureView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;

      void createShaderArgumentsLayout(ResourceHandle handle, ShaderArgumentsLayoutDescriptor& desc) override;
      void createShaderArguments(ResourceHandle handle, ShaderArgumentsDescriptor& binding) override;

      std::shared_ptr<SemaphoreImpl> createSharedSemaphore() override;

      std::shared_ptr<SharedHandle> openSharedHandle(std::shared_ptr<SemaphoreImpl>) override;
      std::shared_ptr<SharedHandle> openSharedHandle(HeapAllocation) override;
      std::shared_ptr<SharedHandle> openSharedHandle(ResourceHandle handle) override;
      std::shared_ptr<SharedHandle> openForInteropt(ResourceHandle resource) override {return nullptr;};
      std::shared_ptr<SemaphoreImpl> createSemaphoreFromHandle(std::shared_ptr<SharedHandle>) override;
      void createHeapFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared) override;
      void createBufferFromHandle(ResourceHandle , std::shared_ptr<SharedHandle>, HeapAllocation, ResourceDescriptor&) override;
      void createTextureFromHandle(ResourceHandle , std::shared_ptr<SharedHandle>, ResourceDescriptor&) override;

      VulkanConstantBuffer allocateConstants(MemView<uint8_t> bytes);

      void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, FormatType format) override;
      void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned stride) override;
      void dynamicImage(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned rowPitch) override;

      void readbackBuffer(ResourceHandle readback, size_t bytes) override;
      MemView<uint8_t> mapReadback(ResourceHandle readback) override;
      void unmapReadback(ResourceHandle readback) override;

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
        std::optional<std::shared_ptr<FenceImpl>>         fence);

      void submitDMA(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        std::optional<std::shared_ptr<FenceImpl>>   fence) override;

      void submitCompute(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        std::optional<std::shared_ptr<FenceImpl>>   fence) override;

      void submitGraphics(
        MemView<std::shared_ptr<CommandBufferImpl>> lists,
        MemView<std::shared_ptr<SemaphoreImpl>>     wait,
        MemView<std::shared_ptr<SemaphoreImpl>>     signal,
        std::optional<std::shared_ptr<FenceImpl>>   fence) override;

      void waitFence(std::shared_ptr<FenceImpl>     fence) override;
      bool checkFence(std::shared_ptr<FenceImpl>    fence) override;

      void present(std::shared_ptr<prototypes::SwapchainImpl> swapchain, std::shared_ptr<SemaphoreImpl> renderingFinished) override;
    };
  }
}