#pragma once
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"

#include "faze/src/new_gfx/vk/util/AllocStuff.hpp" // refactor
#include "faze/src/new_gfx/vk/util/ShaderStorage.hpp"

#include "faze/src/new_gfx/definitions.hpp"

#include <vulkan/vulkan.hpp>

namespace faze
{
  namespace backend
  {
    // implementations
    class VulkanCommandBuffer : public CommandBufferImpl
    {
      vk::CommandBuffer  m_cmdBuffer;
      vk::CommandPool     m_pool;
    public:
      VulkanCommandBuffer(vk::CommandBuffer cmdBuffer, vk::CommandPool pool)
        : m_cmdBuffer(cmdBuffer)
        , m_pool(pool)
      {}

      void fillWith(backend::IntermediateList&) override
      {
        m_cmdBuffer.begin(vk::CommandBufferBeginInfo()
          .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
          .setPInheritanceInfo(nullptr));
        // TODO: move this function somewhere where we have space to actually implement this.
        m_cmdBuffer.end();
      }

      vk::CommandBuffer list()
      {
        return m_cmdBuffer;
      }

      vk::CommandPool pool()
      {
        return m_pool;
      }
    };

    class VulkanSemaphore : public SemaphoreImpl
    {
      vk::Semaphore semaphore;
    public:
      VulkanSemaphore(vk::Semaphore semaphore)
        : semaphore(semaphore)
      {}

      vk::Semaphore native()
      {
        return semaphore;
      }
    };

    class VulkanFence : public FenceImpl
    {
      vk::Fence fence;
    public:
      VulkanFence(vk::Fence fence)
        : fence(fence)
      {}

      vk::Fence native()
      {
        return fence;
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

    class VulkanTexture : public prototypes::TextureImpl
    {
    private:
      vk::Image resource;
      bool owned = true;

    public:
      VulkanTexture()
      {}
      VulkanTexture(vk::Image resource)
        : resource(resource)
      {}

      VulkanTexture(vk::Image resource, bool owner)
        : resource(resource)
        , owned(owner)
      {}
      vk::Image native()
      {
        return resource;
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
        vk::ImageSubresourceRange subResourceRange;
      } m;

    public:
      VulkanTextureView()
      {}
      VulkanTextureView(vk::ImageView view, vk::DescriptorImageInfo info, vk::Format format, vk::DescriptorType viewType, vk::ImageSubresourceRange subResourceRange)
        : m{ view, info, format, viewType, subResourceRange }
      {}
      Info& native()
      {
        return m;
      }
    };

    class VulkanBuffer : public prototypes::BufferImpl
    {
    private:
      vk::Buffer resource;

    public:
      VulkanBuffer()
      {}
      VulkanBuffer(vk::Buffer resource)
        : resource(resource)
      {}
      vk::Buffer native()
      {
        return resource;
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
        : m{info, type}
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

    // helpers

    class GrowingSemaphorePool
    {
      vector<vk::Semaphore> m_semaphores;
      FreelistAllocator m_allocator;
    public:
      GrowingSemaphorePool()
      {
      }
      std::shared_ptr<VulkanSemaphore> allocate(vk::Device device)
      {
        auto index = m_allocator.allocate();
        if (index == -1)
        {
          m_allocator.grow();
          m_semaphores.emplace_back(device.createSemaphore(vk::SemaphoreCreateInfo()));
          index = m_allocator.allocate();
        }
        auto semaphore = m_semaphores[index];

        auto sema = std::shared_ptr<VulkanSemaphore>(new VulkanSemaphore(m_semaphores[index]), [&, index](VulkanSemaphore* semap)
        {
          m_allocator.release(index);
          delete semap;
        });
        return sema;
      }
      void destroy(vk::Device device)
      {
        for (auto&& it : m_semaphores)
        {
          device.destroySemaphore(it);
        }
      }
    };

    class GrowingFencePool
    {
      vector<vk::Fence> m_fences;
      FreelistAllocator m_allocator;
    public:
      GrowingFencePool()
      {
      }
      std::shared_ptr<VulkanFence> allocate(vk::Device device)
      {
        auto index = m_allocator.allocate();
        if (index == -1)
        {
          m_allocator.grow();
          m_fences.emplace_back(device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)));
          index = m_allocator.allocate();
        }

        auto fence = std::shared_ptr<VulkanFence>(new VulkanFence(m_fences[index]), [&, index](VulkanFence* semap)
        {
          m_allocator.release(index);
          delete semap;
        });
        return fence;
      }
      void destroy(vk::Device device)
      {
        for (auto&& it : m_fences)
        {
          device.destroyFence(it);
        }
      }
    };

    class VulkanCommandBufferPool
    {
      vector<VulkanCommandBuffer> m_lists;
      FreelistAllocator m_allocator;
      int queueIndex;
    public:
      VulkanCommandBufferPool()
      {
      }

      VulkanCommandBufferPool(int queueIndex)
        : queueIndex(queueIndex)
      {
      }
      template<typename Device>
      std::shared_ptr<VulkanCommandBuffer>  allocate(Device* device)
      {
        auto index = m_allocator.allocate();
        if (index == -1)
        {
          m_allocator.grow();
          m_lists.emplace_back(device->createCommandBuffer(queueIndex));
          index = m_allocator.allocate();
        }
        else
        {
          device->resetListNative(m_lists[index]);
        }

        auto list = std::shared_ptr<VulkanCommandBuffer>(new VulkanCommandBuffer(m_lists[index]), [&, index](VulkanCommandBuffer* list)
        {
          m_allocator.release(index);
          delete list;
        });
        return list;
      }
      
      void destroy(vk::Device device)
      {
        for (auto&& it : m_lists)
        {
          device.destroyCommandPool(it.pool());
        }
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

      GrowingSemaphorePool        m_semaphores;
      GrowingFencePool            m_fences;

      VulkanCommandBufferPool     m_copyListPool;
      VulkanCommandBufferPool     m_computeListPool;
      VulkanCommandBufferPool     m_graphicsListPool;

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
      /*
      struct MemoryTypes
      {
        int deviceLocalIndex = -1;
        int hostNormalIndex = -1;
        int hostCachedIndex = -1; 
        int deviceHostIndex = -1;
      } m_memoryTypes;
      */
    public:
      VulkanDevice(
        vk::Device device,
        vk::PhysicalDevice physDev,
        FileSystem& fs,
        std::vector<vk::QueueFamilyProperties> queues,
        GpuInfo info,
        bool debugLayer);
      ~VulkanDevice();
      
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
#if defined(FAZE_PLATFORM_WINDOWS)
        , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
        , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
      };

      std::vector<std::string> devExtOrder = {
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