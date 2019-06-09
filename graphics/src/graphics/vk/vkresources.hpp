#pragma once
#include "graphics/vk/vulkan.hpp"
#include "graphics/common/prototypes.hpp"
#include "graphics/common/resources.hpp"
#include "graphics/common/commandpackets.hpp"

#include "graphics/vk/util/AllocStuff.hpp" // refactor
#include "graphics/shaders/ShaderStorage.hpp"

#include "graphics/definitions.hpp"

#include "core/system/MemoryPools.hpp"
#include "core/system/SequenceTracker.hpp"
#include <graphics/common/handle.hpp>

#include <graphics/common/allocators.hpp>

namespace faze
{
  namespace backend
  {
    vk::BufferCreateInfo fillBufferInfo(ResourceDescriptor descriptor);
    vk::ImageCreateInfo fillImageInfo(ResourceDescriptor descriptor);
    int32_t FindProperties(vk::PhysicalDeviceMemoryProperties memprop, uint32_t memoryTypeBits, vk::MemoryPropertyFlags properties);

    struct MemoryPropertySearch
    {
      vk::MemoryPropertyFlags optimal;
      vk::MemoryPropertyFlags def;
    };

    MemoryPropertySearch getMemoryProperties(ResourceUsage usage);

    class VulkanDevice;
    // implementations
    class VulkanCommandList
    {
      vk::CommandBuffer  m_cmdBuffer;
      std::shared_ptr<vk::CommandPool>    m_pool;
    public:
      VulkanCommandList(vk::CommandBuffer cmdBuffer, std::shared_ptr<vk::CommandPool> pool)
        : m_cmdBuffer(cmdBuffer)
        , m_pool(pool)
      {}

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

      bool HDRSupport() override
      {
        return false;
      }

      DisplayCurve displayCurve() override
      {
        return DisplayCurve::sRGB;
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

    class VulkanTexture
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

      backend::TrackedState dependency()
      {
        backend::TrackedState state{};
        state.resPtr = reinterpret_cast<size_t>(&resource);
        state.statePtr = reinterpret_cast<size_t>(statePtr.get());
        state.additionalInfo = 0;
        return state;
      }
    };

    class VulkanTextureView
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

      backend::RawView view()
      {
        return backend::RawView{};
      }
    };

    struct VulkanBufferState
    {
      vk::AccessFlags flags = vk::AccessFlagBits(0);
      int queueIndex = -1;
    };

    class VulkanBuffer
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

      backend::TrackedState dependency()
      {
        return backend::TrackedState{ reinterpret_cast<size_t>(&resource), reinterpret_cast<size_t>(statePtr.get()), 0 };
      }
    };

    class VulkanBufferView
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

      backend::RawView view()
      {
        return backend::RawView{};
      }
    };

    struct VkUploadBlock
    {
      uint8_t* m_data;
      vk::Buffer m_buffer;
      PageBlock block;

      uint8_t* data()
      {
        return m_data + block.offset;
      }

      vk::Buffer buffer()
      {
        return m_buffer;
      }

      size_t size()
      {
        return block.size;
      }

      explicit operator bool() const
      {
        return m_data != nullptr;
      }
    };

    class VulkanDynamicBufferView : public prototypes::DynamicBufferViewImpl
    {
    private:
      struct Info
      {
        vk::Buffer buffer;
        vk::BufferView bufferInfo;
        VkUploadBlock block;
      } m;

    public:
      VulkanDynamicBufferView()
      {}
      VulkanDynamicBufferView(vk::Buffer buffer, vk::BufferView view, VkUploadBlock block)
        : m{ buffer, view, block }
      {}
      Info& native()
      {
        return m;
      }

      backend::RawView view() override
      {
        return backend::RawView{};
      }

      int rowPitch() override
      {
        return -1;
      }

      uint64_t offset() override
      {
        return m.block.block.offset;
      }

      uint64_t size() override
      {
        return m.block.block.size;
      }
    };

    class VulkanHeap
    {
    private:
      vk::DeviceMemory m_resource;

    public:
    VulkanHeap(){}
      VulkanHeap(vk::DeviceMemory memory)
        : m_resource(memory)
      {}

      vk::DeviceMemory native()
      {
        return m_resource;
      }
    };

    class VulkanPipeline
    {
    public:
      vk::Pipeline            m_pipeline;
      bool                    m_hasPipeline;
      vk::DescriptorSetLayout m_descriptorSetLayout;
      vk::PipelineLayout      m_pipelineLayout;
      VulkanPipeline() {}

      VulkanPipeline(vk::DescriptorSetLayout descriptorSetLayout, vk::PipelineLayout pipelineLayout)
        : m_hasPipeline(false)
        , m_descriptorSetLayout(descriptorSetLayout)
        , m_pipelineLayout(pipelineLayout)
      {}

      VulkanPipeline(vk::Pipeline pipeline, vk::DescriptorSetLayout descriptorSetLayout, vk::PipelineLayout pipelineLayout)
        : m_pipeline(pipeline)
        , m_hasPipeline(true)
        , m_descriptorSetLayout(descriptorSetLayout)
        , m_pipelineLayout(pipelineLayout)
      {}
    };

    class VulkanRenderpass
    {
    private:
      vk::RenderPass m_renderpass = nullptr;
      bool is_valid = false;
      std::shared_ptr<unordered_map<int64_t, std::shared_ptr<vk::Framebuffer>>> m_framebuffers;
      size_t m_activeHash = 0;
    public:

      VulkanRenderpass()
        : m_framebuffers(std::make_shared<unordered_map<int64_t, std::shared_ptr<vk::Framebuffer>>>())
       {}

      bool valid() const
      {
        return is_valid;
      }

      void setValid()
      {
        is_valid = true;
      }

      vk::RenderPass& native()
      {
        return m_renderpass;
      }

      unordered_map<int64_t, std::shared_ptr<vk::Framebuffer>>& framebuffers()
      {
        return *m_framebuffers;
      }

      void setActiveFramebuffer(size_t hash)
      {
        m_activeHash = hash;
      }

      vk::Framebuffer getActiveFramebuffer()
      {
        return *(*m_framebuffers)[m_activeHash];
      }
    };

    class VulkanUploadLinearAllocator
    {
      LinearAllocator allocator;
      VkUploadBlock block;

    public:
      VulkanUploadLinearAllocator() {}
      VulkanUploadLinearAllocator(VkUploadBlock block)
        : allocator(block.size())
        , block(block)
      {
      }

      VkUploadBlock allocate(size_t bytes, size_t alignment)
      {
        auto offset = allocator.allocate(bytes, alignment);
        if (offset < 0)
          return VkUploadBlock{ nullptr, nullptr, PageBlock{} };

        VkUploadBlock b = block;
        b.block.offset += offset;
        b.block.size = roundUpMultipleInt(bytes, alignment);
        return b;
      }
    };

    class VulkanUploadHeap
    {
      FixedSizeAllocator allocator;
      unsigned fixedSize = 1;
      unsigned size = 1;
      vk::Buffer m_buffer;
      vk::DeviceMemory m_memory;
      vk::Device m_device;

      uint8_t* data = nullptr;
    public:
      VulkanUploadHeap() : allocator(1, 1) {}
      VulkanUploadHeap(vk::Device device
                     , vk::PhysicalDevice physDevice
                     , unsigned allocationSize
                     , unsigned allocationCount)
        : allocator(allocationSize, allocationCount)
        , fixedSize(allocationSize)
        , size(allocationSize*allocationCount)
        , m_device(device)
      {
        auto bufDesc = ResourceDescriptor()
          .setWidth(allocationSize*allocationCount)
          .setFormat(FormatType::Uint8)
          .setUsage(ResourceUsage::Upload)
          .setName("DynUpload");

        auto natBufferInfo = fillBufferInfo(bufDesc);

        m_buffer = device.createBuffer(natBufferInfo);

        vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(m_buffer);
        auto memProp = physDevice.getMemoryProperties();
        auto searchProperties = getMemoryProperties(bufDesc.desc.usage);
        auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
        F_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?");

        vk::MemoryAllocateInfo allocInfo;

        allocInfo = vk::MemoryAllocateInfo()
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        m_memory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(m_buffer, m_memory, 0);
        // we should have cpu UploadBuffer created above and bound, lets map it.

        data = reinterpret_cast<uint8_t*>(device.mapMemory(m_memory, 0, allocationSize*allocationCount));
      }

      ~VulkanUploadHeap()
      {
        m_device.destroyBuffer(m_buffer);
        m_device.freeMemory(m_memory);
      }

      VkUploadBlock allocate(size_t bytes)
      {
        auto dip = allocator.allocate(bytes);
        F_ASSERT(dip.offset != -1, "No space left, make bigger VulkanUploadHeap :) %d", size);
        return VkUploadBlock{ data, m_buffer,  dip };
      }

      void release(VkUploadBlock desc)
      {
        allocator.release(desc.block);
      }
    };

    class VulkanCommandBuffer : public CommandBufferImpl
    {
      std::shared_ptr<VulkanCommandList> m_list;

    public:
      VulkanCommandBuffer(std::shared_ptr<VulkanCommandList> list)
        : m_list(list)
      {}
    private:
      //void handleRenderpass(VulkanDevice* device, backend::IntermediateList&, CommandPacket* begin, CommandPacket* end);
      //void preprocess(VulkanDevice* device, backend::IntermediateList& list);
    public:
      void fillWith(std::shared_ptr<prototypes::DeviceImpl>, backend::IntermediateList&) override;

      vk::CommandBuffer list()
      {
        return m_list->list();
      }
    };

  }
}