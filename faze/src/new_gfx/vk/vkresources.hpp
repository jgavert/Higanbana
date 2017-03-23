#pragma once
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"

#include "faze/src/new_gfx/vk/util/AllocStuff.hpp" // refactor
#include "faze/src/new_gfx/vk/util/ShaderStorage.hpp"

#include "faze/src/new_gfx/definitions.hpp"

#include "core/src/system/MemoryPools.hpp"

#include <vulkan/vulkan.hpp>

namespace faze
{
  namespace backend
  {
    // implementations
    class VulkanCommandBuffer : public CommandBufferImpl
    {
      vk::CommandBuffer  m_cmdBuffer;
      std::shared_ptr<vk::CommandPool>    m_pool;
    public:
      VulkanCommandBuffer(vk::CommandBuffer cmdBuffer, std::shared_ptr<vk::CommandPool> pool)
        : m_cmdBuffer(cmdBuffer)
        , m_pool(pool)
      {}

      void fillWith(std::shared_ptr<prototypes::DeviceImpl>, backend::IntermediateList&) override;

      vk::CommandBuffer list()
      {
        return m_cmdBuffer;
      }

      vk::CommandPool pool()
      {
        return *m_pool;
      }
    };

    class VulkanSemaphore : public SemaphoreImpl
    {
      std::shared_ptr<vk::Semaphore> semaphore;
    public:
      VulkanSemaphore(std::shared_ptr<vk::Semaphore> semaphore)
        : semaphore(semaphore)
      {}

      vk::Semaphore native()
      {
        return *semaphore;
      }
    };

    class VulkanFence : public FenceImpl
    {
      std::shared_ptr<vk::Fence> fence;
    public:
      VulkanFence(std::shared_ptr<vk::Fence> fence)
        : fence(fence)
      {}

      vk::Fence native()
      {
        return *fence;
      }
    };

    class VulkanGraphicsSurface : public prototypes::GraphicsSurfaceImpl
    {
    private:
      std::shared_ptr<vk::SurfaceKHR> surface;

    public:
      VulkanGraphicsSurface()
      {}
      VulkanGraphicsSurface(std::shared_ptr<vk::SurfaceKHR> surface)
        : surface(surface)
      {}
      vk::SurfaceKHR native()
      {
        return *surface;
      }
    };

    class VulkanSwapchain : public prototypes::SwapchainImpl
    {
      vk::SwapchainKHR m_swapchain;
      VulkanGraphicsSurface m_surface;
      std::shared_ptr<VulkanSemaphore> m_acquireSemaphore = nullptr;
      std::shared_ptr<VulkanSemaphore> renderingFinished = nullptr;

      int m_index = -1;
      struct Desc
      {
        int width = 0;
        int height = 0;
        int buffers = 0;
        FormatType format = FormatType::Unknown;
        PresentMode mode = PresentMode::Unknown;
      } m_desc;

    public:

      VulkanSwapchain()
      {}
      VulkanSwapchain(vk::SwapchainKHR resource, VulkanGraphicsSurface surface, std::shared_ptr<VulkanSemaphore> semaphore)
        : m_swapchain(resource)
        , m_surface(surface)
        , renderingFinished(semaphore)
      {}

      ResourceDescriptor desc() override
      {
        return ResourceDescriptor()
          .setWidth(m_desc.width)
          .setHeight(m_desc.height)
          .setFormat(m_desc.format)
          .setUsage(ResourceUsage::RenderTarget)
          .setMiplevels(1)
          .setArraySize(1)
          .setName("Swapchain Image")
          .setDepth(1);
      }

      int getCurrentPresentableImageIndex() override
      {
        return m_index;
      }

      std::shared_ptr<SemaphoreImpl> acquireSemaphore() override
      {
        return m_acquireSemaphore;
      }

      std::shared_ptr<backend::SemaphoreImpl> renderSemaphore() override
      {
        return renderingFinished;
      }

      void setAcquireSemaphore(std::shared_ptr<VulkanSemaphore> semaphore)
      {
        m_acquireSemaphore = semaphore;
      }
      void setBufferMetadata(int x, int y, int count, FormatType format, PresentMode mode)
      {
        m_desc.width = x;
        m_desc.height = y;
        m_desc.buffers = count;
        m_desc.format = format;
        m_desc.mode = mode;
      }

      Desc getDesc()
      {
        return m_desc;
      }

      void setCurrentPresentableImageIndex(int index)
      {
        m_index = index;
      }

      void setSwapchain(vk::SwapchainKHR chain)
      {
        m_swapchain = chain;
      }

      vk::SwapchainKHR native()
      {
        return m_swapchain;
      }

      VulkanGraphicsSurface& surface()
      {
        return m_surface;
      }
    };

    struct TextureStateFlags
    {
      vk::AccessFlags access;
      vk::ImageLayout layout;
      int queueIndex = -1;
      TextureStateFlags(vk::AccessFlags access, vk::ImageLayout layout, int queueIndex)
        : access(access)
        , layout(layout)
        , queueIndex(queueIndex)
      {}
    };

    struct VulkanTextureState
    {
      vector<TextureStateFlags> flags; // subresource count
    };

    class VulkanTexture : public prototypes::TextureImpl
    {
    private:
      vk::Image resource;
      std::shared_ptr<VulkanTextureState> statePtr;
      bool owned;

    public:
      VulkanTexture()
      {}

      VulkanTexture(vk::Image resource, std::shared_ptr<VulkanTextureState> state, bool owner = true)
        : resource(resource)
        , statePtr(state)
        , owned(owner)
      {}
      vk::Image native()
      {
        return resource;
      }

      std::shared_ptr<VulkanTextureState> state()
      {
        return statePtr;
      }

      bool canRelease()
      {
        return owned;
      }
    };

    class VulkanTextureView : public prototypes::TextureViewImpl
    {
    private:
      struct Info
      {
        vk::ImageView view;
        vk::DescriptorImageInfo info;
        vk::Format format;
        vk::DescriptorType viewType;
        SubresourceRange subResourceRange;
        vk::ImageAspectFlags aspectMask;
      } m;

    public:
      VulkanTextureView()
      {}
      VulkanTextureView(vk::ImageView view, vk::DescriptorImageInfo info, vk::Format format, vk::DescriptorType viewType, SubresourceRange subResourceRange, vk::ImageAspectFlags aspectMask)
        : m{ view, info, format, viewType, subResourceRange, aspectMask }
      {}
      Info& native()
      {
        return m;
      }

      vk::ImageSubresourceRange range()
      {
        return vk::ImageSubresourceRange()
          .setAspectMask(m.aspectMask)
          .setBaseMipLevel(m.subResourceRange.mipOffset)
          .setLevelCount(m.subResourceRange.mipLevels)
          .setBaseArrayLayer(m.subResourceRange.sliceOffset)
          .setLayerCount(m.subResourceRange.arraySize);
      }
    };

    struct VulkanBufferState
    {
      vk::AccessFlags flags = vk::AccessFlagBits(0);
      int queueIndex = -1;
    };

    class VulkanBuffer : public prototypes::BufferImpl
    {
    private:
      vk::Buffer resource;
      std::shared_ptr<VulkanBufferState> statePtr;

    public:
      VulkanBuffer()
      {}
      VulkanBuffer(vk::Buffer resource, std::shared_ptr<VulkanBufferState> state)
        : resource(resource)
        , statePtr(state)
      {}
      vk::Buffer native()
      {
        return resource;
      }

      std::shared_ptr<VulkanBufferState> state()
      {
        return statePtr;
      }
    };

    class VulkanBufferView : public prototypes::BufferViewImpl
    {
    private:
      struct Info
      {
        vk::DescriptorBufferInfo bufferInfo;
        vk::DescriptorType type;
      } m;

    public:
      VulkanBufferView()
      {}
      VulkanBufferView(vk::DescriptorBufferInfo info, vk::DescriptorType type)
        : m{ info, type }
      {}
      Info& native()
      {
        return m;
      }
    };

    class VulkanHeap : public prototypes::HeapImpl
    {
    private:
      vk::DeviceMemory m_resource;

    public:
      VulkanHeap(vk::DeviceMemory memory)
        : m_resource(memory)
      {}

      vk::DeviceMemory native()
      {
        return m_resource;
      }
    };

    class VulkanRenderpass : public prototypes::RenderpassImpl
    {
    private:
      std::shared_ptr<vk::RenderPass> m_renderpass = nullptr;
      unordered_map<int64_t, std::shared_ptr<vk::Framebuffer>> m_framebuffers;
      size_t m_activeHash = 0;
    public:

      VulkanRenderpass() {}

      bool valid()
      {
        return m_renderpass.get() != nullptr;
      }

      std::shared_ptr<vk::RenderPass>& native()
      {
        return m_renderpass;
      }

      unordered_map<int64_t, std::shared_ptr<vk::Framebuffer>>& framebuffers()
      {
        return m_framebuffers;
      }

      void setActiveFramebuffer(size_t hash)
      {
        m_activeHash = hash;
      }

      vk::Framebuffer getActiveFramebuffer()
      {
        return *m_framebuffers[m_activeHash];
      }
    };

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

      Rabbitpool2<VulkanSemaphore>      m_semaphores;
      Rabbitpool2<VulkanFence>          m_fences;
      Rabbitpool2<VulkanCommandBuffer>  m_copyListPool;
      Rabbitpool2<VulkanCommandBuffer>  m_computeListPool;
      Rabbitpool2<VulkanCommandBuffer>  m_graphicsListPool;

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

      vk::BufferCreateInfo fillBufferInfo(ResourceDescriptor descriptor);
      vk::ImageCreateInfo fillImageInfo(ResourceDescriptor descriptor);

      // implementation
      std::shared_ptr<prototypes::SwapchainImpl> createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int bufferCount) override;
      void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, PresentMode mode, FormatType format, int bufferCount) override;
      void destroySwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      vector<std::shared_ptr<prototypes::TextureImpl>> getSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc) override;
      int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) override;

      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      std::shared_ptr<prototypes::RenderpassImpl> createRenderpass() override;

      GpuHeap createHeap(HeapDescriptor desc) override;
      void destroyHeap(GpuHeap heap) override;

      std::shared_ptr<prototypes::BufferImpl> createBuffer(HeapAllocation allocation, ResourceDescriptor& desc) override;
      void destroyBuffer(std::shared_ptr<prototypes::BufferImpl> buffer) override;

      std::shared_ptr<prototypes::BufferViewImpl> createBufferView(std::shared_ptr<prototypes::BufferImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void destroyBufferView(std::shared_ptr<prototypes::BufferViewImpl> buffer) override;

      std::shared_ptr<prototypes::TextureImpl> createTexture(HeapAllocation allocation, ResourceDescriptor& desc) override;
      void destroyTexture(std::shared_ptr<prototypes::TextureImpl> buffer) override;

      std::shared_ptr<prototypes::TextureViewImpl> createTextureView(std::shared_ptr<prototypes::TextureImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) override;
      void destroyTextureView(std::shared_ptr<prototypes::TextureViewImpl> buffer) override;

      // commandlist stuff
      VulkanCommandBuffer createCommandBuffer(int queueIndex);
      void resetListNative(VulkanCommandBuffer list);
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

    class VulkanSubsystem : public prototypes::SubsystemImpl
    {
      vector<GpuInfo> infos;

      allocs m_allocs;
      vk::AllocationCallbacks m_alloc_info;
      vk::ApplicationInfo app_info;
      vk::InstanceCreateInfo instance_info;
      vector<vk::ExtensionProperties> m_extensions;
      vector<vk::LayerProperties> m_layers;
      vector<vk::PhysicalDevice> m_devices;
      std::shared_ptr<vk::Instance> m_instance;
      std::shared_ptr<vk::DebugReportCallbackEXT> m_debugcallback;

      // lunargvalidation list order
      std::vector<std::string> layerOrder = {
  #if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
          "VK_LAYER_LUNARG_standard_validation",
  #endif
          "VK_LAYER_LUNARG_swapchain"
      };

      std::vector<std::string> extOrder = {
        VK_KHR_SURFACE_EXTENSION_NAME
        , VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
  #if defined(FAZE_PLATFORM_WINDOWS)
          , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
  #endif
  #if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
          , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
  #endif
      };

      std::vector<std::string> devExtOrder = {
        "VK_EXT_shader_subgroup_ballot",
        "VK_EXT_shader_subgroup_vote",
        "VK_KHR_maintenance1",
        "VK_KHR_swapchain"
      };
    public:
      VulkanSubsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
      GpuDevice createGpuDevice(FileSystem& fs, GpuInfo gpu);
      GraphicsSurface createSurface(Window& window) override;
    };
  }
}