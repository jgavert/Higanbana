#pragma once
#include "higanbana/graphics/vk/vulkan.hpp"
#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/command_packets.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/vk/util/AllocStuff.hpp" // refactor
#include "higanbana/graphics/shaders/ShaderStorage.hpp"
#include "higanbana/graphics/definitions.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/allocators.hpp"
#include <higanbana/core/system/MemoryPools.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <higanbana/core/system/heap_allocator.hpp>

#include <algorithm>
#include <mutex>

#define VK_CHECK_RESULT(value) HIGAN_ASSERT(value.result == vk::Result::eSuccess, "Result was not success: \"%s\"", vk::to_string(value.result).c_str())
#define VK_CHECK_RESULT_RAW(value) HIGAN_ASSERT(value == vk::Result::eSuccess, "Result was not success: \"%s\"", vk::to_string(value).c_str())


namespace higanbana
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
      bool m_outOfDate = false;
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

      void setOutOfDate(bool value = true)
      {
        m_outOfDate = value;
      }

      bool outOfDate() override
      {
        return m_outOfDate;
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
      vk::ImageAspectFlags m_aspectFlags; // this probably belongs to ... view :(
      ResourceDescriptor m_desc;
      bool owned;

      vk::DeviceMemory deviceMem;
      bool isDedicatedAllocation;
    public:
      VulkanTexture()
      {}

      VulkanTexture(vk::Image resource, ResourceDescriptor desc, bool owner = true)
        : resource(resource)
        , m_desc(desc)
        , owned(owner)
        , m_aspectFlags(vk::ImageAspectFlagBits::eColor)
        , isDedicatedAllocation(false)
      {
        if (m_desc.desc.usage == ResourceUsage::DepthStencil 
         || m_desc.desc.usage == ResourceUsage::DepthStencilRW)
        {
          if (desc.desc.format == FormatType::Depth32_Stencil8)
            m_aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
          else
            m_aspectFlags = vk::ImageAspectFlagBits::eDepth;
        }
      }

      VulkanTexture(vk::Image resource, ResourceDescriptor desc, vk::DeviceMemory mem, bool owner = true)
        : resource(resource)
        , m_desc(desc)
        , owned(owner)
        , m_aspectFlags(vk::ImageAspectFlagBits::eColor)
        , deviceMem(mem)
        , isDedicatedAllocation(true)
      {
        if (m_desc.desc.usage == ResourceUsage::DepthStencil 
         || m_desc.desc.usage == ResourceUsage::DepthStencilRW)
        {
          if (desc.desc.format == FormatType::Depth32_Stencil8)
            m_aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
          else
            m_aspectFlags = vk::ImageAspectFlagBits::eDepth;
        }
      }

      vk::Image native()
      {
        return resource;
      }

      vk::DeviceMemory memory()
      {
        return deviceMem;
      }

      explicit operator bool()
      {
        return resource && (isDedicatedAllocation == ((bool)deviceMem));
      }

      bool hasDedicatedMemory() const
      {
        return isDedicatedAllocation;
      }

      bool canRelease()
      {
        return owned;
      }

      const ResourceDescriptor& desc() const
      {
        return m_desc;
      }

      vk::ImageAspectFlags aspectFlags()
      {
        return m_aspectFlags;
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

      explicit operator bool()
      {
        return m.view;
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
      vk::DeviceMemory memory;
      bool sharedMemory;

    public:
      VulkanBuffer()
      {}
      VulkanBuffer(vk::Buffer resource)
        : resource(resource)
        , sharedMemory(false)
      {}
      VulkanBuffer(vk::Buffer resource, vk::DeviceMemory memory)
        : resource(resource)
        , memory(memory)
        , sharedMemory(true)
      {}
      vk::Buffer native()
      {
        return resource;
      }

      vk::DeviceMemory sharedMem()
      {
        return memory;
      }

      explicit operator bool()
      {
        return resource && (sharedMemory == ((bool)memory));
      }
    };

    class VulkanBufferView
    {
    private:
      struct Info
      {
        vk::BufferView view;
        vk::DescriptorBufferInfo bufferInfo;
        vk::DescriptorType type;
        vk::Buffer indexBuffer;
        vk::DeviceSize offset;
        vk::IndexType indexType;
      } m;

    public:
      VulkanBufferView()
      {}
      VulkanBufferView(vk::BufferView view, vk::DescriptorType type)
        : m{ view , {}, type, {}, {}, {}}
      {}
      VulkanBufferView(vk::DescriptorBufferInfo view, vk::DescriptorType type)
        : m{ {} , view, type, {}, {}, {}}
      {}
      VulkanBufferView(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType type)
        : m{ {} , {}, {}, buffer, offset, type}
      {}
      Info& native()
      {
        return m;
      }
      explicit operator bool()
      {
        return m.view;
      }
    };

    struct VkUploadBlock
    {
      uint8_t* m_data;
      vk::Buffer m_buffer;
      RangeBlock block;
      int alignOffset;

      uint8_t* data()
      {
        return m_data + block.offset + alignOffset;
      }

      vk::Buffer buffer()
      {
        return m_buffer;
      }

      size_t offset()
      {
        return block.offset + alignOffset;
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

    class VulkanConstantBuffer
    {
      struct Info
      {
        vk::DescriptorBufferInfo bufferInfo;
        VkUploadBlock block;
      } m;
      public:
      VulkanConstantBuffer(vk::DescriptorBufferInfo view, VkUploadBlock block)
        : m{ view, block }
      {}
      Info& native()
      {
        return m;
      }
    };

    class VulkanDynamicBufferView
    {
    private:
      struct Info
      {
        vk::Buffer buffer;
        vk::BufferView texelView;
        vk::DescriptorBufferInfo bufferInfo;
        vk::DescriptorType type;
        vk::IndexType index;
        VkUploadBlock block;
        unsigned rowPitch;
      } m;

    public:
      VulkanDynamicBufferView()
      {}
      VulkanDynamicBufferView(vk::Buffer buffer, vk::BufferView view, VkUploadBlock block, vk::IndexType indextype)
        : m{ buffer, view, {}, vk::DescriptorType::eUniformTexelBuffer, indextype, block, 0 }
      {}
      VulkanDynamicBufferView(vk::Buffer buffer, vk::DescriptorBufferInfo view, VkUploadBlock block)
        : m{ buffer, {}, view, vk::DescriptorType::eStorageBuffer, vk::IndexType::eUint16, block, 0 }
      {}
      VulkanDynamicBufferView(vk::Buffer buffer, vk::BufferView texelView, vk::DescriptorBufferInfo view, VkUploadBlock block)
        : m{ buffer, texelView, view, vk::DescriptorType::eStorageBuffer, vk::IndexType::eUint16, block, 0 }
      {}
      VulkanDynamicBufferView(vk::Buffer buffer, vk::DescriptorBufferInfo view, VkUploadBlock block, unsigned rowPitch)
        : m{ buffer, {}, view, vk::DescriptorType::eStorageBuffer, vk::IndexType::eUint16, block, rowPitch }
      {}
      Info& native()
      {
        return m;
      }

      size_t indexOffset()
      {
        return m.block.offset();
      }

      explicit operator bool()
      {
        return m.texelView;
      }
    };

    class VulkanReadback
    {
      vk::DeviceMemory m_memory;
      vk::Buffer m_buffer;
      size_t m_offset;
      size_t m_size;
    public:
      VulkanReadback()
        : m_offset(0)
        , m_size(0)
      {}
      VulkanReadback(vk::DeviceMemory memory, vk::Buffer buffer, size_t offset, size_t size)
        : m_memory(memory)
        , m_buffer(buffer)
        , m_offset(offset)
        , m_size(size)
      {}
      
      vk::Buffer native()
      {
        return m_buffer;
      }
      vk::DeviceMemory memory()
      {
        return m_memory;
      }
      size_t offset() const
      {
        return m_offset;
      }
      size_t size() const
      {
        return m_size;
      }

      explicit operator bool()
      {
        return m_memory || m_buffer;
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
      explicit operator bool()
      {
        return m_resource;
      }
    };

    class VulkanShaderArgumentsLayout
    {
    private:
      vk::DescriptorSetLayout m_resource;

    public:
      VulkanShaderArgumentsLayout(){}
      VulkanShaderArgumentsLayout(vk::DescriptorSetLayout layout)
        : m_resource(layout)
      {}

      vk::DescriptorSetLayout native()
      {
        return m_resource;
      }
      explicit operator bool()
      {
        return m_resource;
      }
    };

    class VulkanPipeline
    {
    public:
      vk::Pipeline            m_pipeline;
      std::shared_ptr<std::atomic<bool>> m_hasPipeline;
      vk::PipelineLayout      m_pipelineLayout;
      vk::DescriptorSet       m_staticSet;
      GraphicsPipelineDescriptor m_gfxDesc;
      ComputePipelineDescriptor m_computeDesc;

      vector<std::pair<WatchFile, ShaderType>> m_watchedShaders;

      WatchFile cs;
      bool needsUpdating() noexcept
      {
        return std::any_of(m_watchedShaders.begin(), m_watchedShaders.end(), [](std::pair<WatchFile, ShaderType>& shader){ return shader.first.updated();});
      }

      void updated()
      {
        std::for_each(m_watchedShaders.begin(), m_watchedShaders.end(), [](std::pair<WatchFile, ShaderType>& shader){ shader.first.react();});
      }

      VulkanPipeline() {}

      VulkanPipeline(vk::PipelineLayout pipelineLayout, GraphicsPipelineDescriptor gfxDesc, vk::DescriptorSet set)
        : m_hasPipeline(std::make_shared<std::atomic<bool>>(false))
        , m_pipelineLayout(pipelineLayout)
        , m_staticSet(set)
        , m_gfxDesc(gfxDesc)
      {}

      VulkanPipeline(vk::PipelineLayout pipelineLayout, ComputePipelineDescriptor computeDesc, vk::DescriptorSet set)
        : m_hasPipeline(std::make_shared<std::atomic<bool>>(false))
        , m_pipelineLayout(pipelineLayout)
        , m_staticSet(set)
        , m_computeDesc(computeDesc)
      {}

      VulkanPipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, ComputePipelineDescriptor computeDesc, vk::DescriptorSet set)
        : m_pipeline(pipeline)
        , m_hasPipeline(std::make_shared<std::atomic<bool>>(true))
        , m_pipelineLayout(pipelineLayout)
        , m_staticSet(set)
        , m_computeDesc(computeDesc)
      {}
      
      explicit operator bool()
      {
        return m_pipelineLayout || m_pipeline || m_staticSet;
      }
    };

    class VulkanRenderpass
    {
    private:
      vk::RenderPass m_renderpass = nullptr;
      std::shared_ptr<std::atomic<bool>> is_valid = nullptr;
      size_t m_activeHash = 0;
    public:

      VulkanRenderpass()
        : is_valid(std::make_shared<std::atomic<bool>>(false))
       {}

      bool valid() const
      {
        return is_valid->load();
      }

      void setValid()
      {
        is_valid->store(true);
      }

      vk::RenderPass& native()
      {
        return m_renderpass;
      }

      explicit operator bool()
      {
        return m_renderpass;
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
          return VkUploadBlock{ nullptr, nullptr, RangeBlock{} };

        VkUploadBlock b = block;
        b.block.offset += offset;
        b.block.size = roundUpMultipleInt(bytes, alignment);
        return b;
      }
    };

    class VulkanUploadHeap
    {
      HeapAllocator allocator;
      vk::Buffer m_buffer;
      vk::DeviceMemory m_memory;
      vk::Device m_device;

      uint8_t* data = nullptr;
      std::mutex uploadLock;
    public:
      VulkanUploadHeap() : allocator(1, 1) {}
      VulkanUploadHeap(vk::Device device
                     , vk::PhysicalDevice physDevice
                     , unsigned memorySize
                     , unsigned unaccessibleMemoryAtEnd = 0)
        : allocator(memorySize-unaccessibleMemoryAtEnd)
        , m_device(device)
      {
        auto bufDesc = ResourceDescriptor()
          .setWidth(memorySize)
          .setFormat(FormatType::Uint8)
          .setUsage(ResourceUsage::Upload)
          .setIndexBuffer()
          .setName("DynUpload");

        auto natBufferInfo = fillBufferInfo(bufDesc);

        auto result = device.createBuffer(natBufferInfo);
        if (result.result == vk::Result::eSuccess)
        {
          m_buffer = result.value;
        }

        vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(m_buffer);
        auto memProp = physDevice.getMemoryProperties();
        auto searchProperties = getMemoryProperties(bufDesc.desc.usage);
        auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
        HIGAN_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?");

        vk::MemoryAllocateInfo allocInfo;

        allocInfo = vk::MemoryAllocateInfo()
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        auto allocRes = device.allocateMemory(allocInfo);
        VK_CHECK_RESULT(allocRes);
        m_memory = allocRes.value;
        device.bindBufferMemory(m_buffer, m_memory, 0);
        // we should have cpu UploadBuffer created above and bound, lets map it.

        auto mapResult = device.mapMemory(m_memory, 0, memorySize);
        VK_CHECK_RESULT(mapResult);

        data = reinterpret_cast<uint8_t*>(mapResult.value);
      }

      ~VulkanUploadHeap()
      {
        m_device.destroyBuffer(m_buffer);
        m_device.freeMemory(m_memory);
      }

      VkUploadBlock allocate(size_t bytes, size_t alignment = 1)
      {
        std::lock_guard<std::mutex> lock(uploadLock);
        auto dip = allocator.allocate(bytes, alignment);
        HIGAN_ASSERT(dip, "No space left, make bigger VulkanUploadHeap :) %d", allocator.max_size());
        return VkUploadBlock{ data, m_buffer,  dip.value()};
      }

      void release(VkUploadBlock desc)
      {
        std::lock_guard<std::mutex> lock(uploadLock);
        allocator.free(desc.block);
      }

      size_t size() const noexcept
      {
        return allocator.size();
      }

      size_t max_size() const noexcept
      {
        return allocator.max_size();
      }

      size_t size_allocated() const noexcept
      {
        return allocator.size_allocated();
      }
    };

    class VulkanConstantUploadHeap
    {
      HeapAllocator allocator;
      vk::Buffer m_buffer;
      vk::DeviceMemory m_memory;
      vk::Device m_device;
      unsigned minUniformBufferAlignment;

      uint8_t* data = nullptr;
      std::mutex m_allocatorBlocker;
    public:
      VulkanConstantUploadHeap() : allocator(1, 1) {}
      VulkanConstantUploadHeap(vk::Device device
                     , vk::PhysicalDevice physDevice
                     , unsigned memorySize)
        : allocator(memorySize)
        , m_device(device)
        , minUniformBufferAlignment(physDevice.getProperties().limits.minUniformBufferOffsetAlignment)
      {
        auto bufDesc = ResourceDescriptor()
          .setWidth(memorySize)
          .setFormat(FormatType::Uint8)
          .setUsage(ResourceUsage::Upload)
          .setName("DynUpload");

        auto natBufferInfo = fillBufferInfo(bufDesc);
        natBufferInfo = natBufferInfo.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

        auto result = device.createBuffer(natBufferInfo);
        if (result.result == vk::Result::eSuccess)
        {
          m_buffer = result.value;
        }

        vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(m_buffer);
        auto memProp = physDevice.getMemoryProperties();
        auto searchProperties = getMemoryProperties(bufDesc.desc.usage);
        auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
        HIGAN_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?");

        vk::MemoryAllocateInfo allocInfo;

        allocInfo = vk::MemoryAllocateInfo()
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        auto allocRes = device.allocateMemory(allocInfo);
        VK_CHECK_RESULT(allocRes);
        m_memory = allocRes.value;
        device.bindBufferMemory(m_buffer, m_memory, 0);
        // we should have cpu UploadBuffer created above and bound, lets map it.

        auto mapResult = device.mapMemory(m_memory, 0, memorySize);
        VK_CHECK_RESULT(mapResult);

        data = reinterpret_cast<uint8_t*>(mapResult.value);
      }

      ~VulkanConstantUploadHeap()
      {
        m_device.destroyBuffer(m_buffer);
        m_device.freeMemory(m_memory);
      }

      VkUploadBlock allocate(size_t bytes)
      {
        std::lock_guard<std::mutex> guard(m_allocatorBlocker);
        auto dip = allocator.allocate(bytes, minUniformBufferAlignment);
        HIGAN_ASSERT(dip, "No space left, make bigger VulkanUploadHeap :) %d", allocator.max_size());
        return VkUploadBlock{ data, m_buffer,  dip.value() };
      }

      void release(VkUploadBlock desc)
      {
        std::lock_guard<std::mutex> guard(m_allocatorBlocker);
        allocator.free(desc.block);
      }

      void releaseRange(vector<VkUploadBlock>& descs)
      {
        std::lock_guard<std::mutex> guard(m_allocatorBlocker);
        for (auto&& desc : descs)
          allocator.free(desc.block);
      }

      vk::Buffer buffer()
      {
        return m_buffer;
      }

      unsigned allocationAlignment() const noexcept
      {
        return minUniformBufferAlignment;
      }

      size_t size() const noexcept
      {
        return allocator.size();
      }

      size_t max_size() const noexcept
      {
        return allocator.max_size();
      }

      size_t size_allocated() const noexcept
      {
        return allocator.size_allocated();
      }
    };

    class VkUploadLinearAllocator
    {
      LinearAllocator allocator;
      VkUploadBlock block;

    public:
      VkUploadLinearAllocator() {}
      VkUploadLinearAllocator(VkUploadBlock block)
        : allocator(block.size())
        , block(block)
      {
      }

      VkUploadBlock allocate(size_t bytes, size_t alignment)
      {
        auto offset = allocator.allocate(bytes, alignment);
        if (offset < 0)
          return VkUploadBlock{ nullptr, vk::Buffer{}, RangeBlock{} };

        VkUploadBlock b = block;
        b.block.offset += offset;
        b.block.size = roundUpMultipleInt(bytes, alignment);
        return b;
      }
    };

    class VulkanDescriptorPool
    {
      vk::DescriptorPool pool;
      std::mutex descriptorLock;
      public:
      VulkanDescriptorPool()
      {
      }
      VulkanDescriptorPool(vk::DescriptorPool pool)
        :pool(pool)
      {
      }
      std::vector<vk::DescriptorSet> allocate(vk::Device device, vk::DescriptorSetLayout layout, int count)
      {
        std::lock_guard<std::mutex> poolLock(descriptorLock);
        auto res = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
          .setDescriptorPool(pool)
          .setDescriptorSetCount(count)
          .setPSetLayouts(&layout));
        VK_CHECK_RESULT(res);
        return res.value;
      }
      void freeSets(vk::Device device, MemView<vk::DescriptorSet> sets)
      {
        if (!sets.empty())
        {
          std::lock_guard<std::mutex> poolLock(descriptorLock);
          vk::ArrayProxy<const vk::DescriptorSet> csets(sets.size(), sets.data());
          device.freeDescriptorSets(pool, csets);
        }
      }
      vk::DescriptorPool native()
      {
        return pool;
      }

      explicit operator bool()
      {
        return pool;
      }
    };

    class VulkanShaderArguments
    {
      vk::DescriptorSet m_set;
    public:
      VulkanShaderArguments(){}
      VulkanShaderArguments(vk::DescriptorSet set)
        : m_set(set)
      {}
      vk::DescriptorSet native()
      {
        return m_set;
      }
      explicit operator bool()
      {
        return m_set;
      }
    };

    struct Resources
    {
      HandleVector<VulkanTexture> tex;
      HandleVector<VulkanBuffer> buf;
      HandleVector<VulkanBufferView> bufSRV;
      HandleVector<VulkanBufferView> bufUAV;
      HandleVector<VulkanBufferView> bufIBV;
      HandleVector<VulkanDynamicBufferView> dynBuf;
      HandleVector<VulkanReadback>          rbBuf;
      HandleVector<VulkanTextureView> texSRV;
      HandleVector<VulkanTextureView> texUAV;
      HandleVector<VulkanTextureView> texRTV;
      HandleVector<VulkanTextureView> texDSV;
      HandleVector<VulkanPipeline> pipelines;
      HandleVector<VulkanRenderpass> renderpasses;
      HandleVector<VulkanHeap> heaps;
      HandleVector<VulkanShaderArguments> shaArgs;
      HandleVector<VulkanShaderArgumentsLayout> shaArgsLayouts;
    };

    struct QueueIndexes
    {
      int graphics;
      int compute;
      int dma;

      int queue(QueueType type) const
      {
        switch (type)
        {
          case QueueType::Graphics: return graphics;
          case QueueType::Compute: return compute;
          case QueueType::Dma: return dma;
          case QueueType::External: return VK_QUEUE_FAMILY_EXTERNAL;
          case QueueType::Unknown:
          default: return VK_QUEUE_FAMILY_IGNORED;
        }
      }

    };

    struct StaticSamplers
    {
      vk::Sampler                 m_bilinearSampler;
      vk::Sampler                 m_pointSampler;
      vk::Sampler                 m_bilinearSamplerWrap;
      vk::Sampler                 m_pointSamplerWrap;
    };

    class VulkanCommandBuffer : public CommandBufferImpl
    {
      std::shared_ptr<VulkanCommandList> m_list;
      vector<std::shared_ptr<vk::Framebuffer>> m_framebuffers;
      //VulkanDescriptorPool m_descriptors;
      std::shared_ptr<VulkanConstantUploadHeap> m_constants;
      vector<VkUploadBlock> m_allocatedConstants;
      vector<vk::Pipeline> m_oldPipelines;
      vector<vk::DescriptorSet> m_tempSets;

      ResourceHandle m_boundDescriptorSets[4]; // yeah, lets keep it at 4 for now...
      unsigned m_constantAlignment;

      VkUploadLinearAllocator m_constantsAllocator;

    public:
      VulkanCommandBuffer(std::shared_ptr<VulkanCommandList> list, std::shared_ptr<VulkanDescriptorPool> descriptors, std::shared_ptr<VulkanConstantUploadHeap> constantAllocators)
        : m_list(list), /*m_descriptors(descriptors),*/ m_constants(constantAllocators), m_constantAlignment(m_constants->allocationAlignment())
      {}
    private:
      void handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBinding& packet, ResourceHandle pipeline);
      void addCommands(VulkanDevice* device, vk::CommandBuffer buffer, MemView<backend::CommandBuffer>& buffers, BarrierSolver& solver);
      void handleRenderpass(VulkanDevice* device, gfxpacket::RenderPassBegin& renderpasspacket);
      void preprocess(VulkanDevice* device, MemView<backend::CommandBuffer>& list);
      VkUploadBlock allocateConstants(size_t size);
    public:
      void fillWith(std::shared_ptr<prototypes::DeviceImpl>, MemView<backend::CommandBuffer>& buffers, BarrierSolver& solver) override;
      void readbackTimestamps(std::shared_ptr<prototypes::DeviceImpl>, vector<GraphNodeTiming>& nodes) override;

      vk::CommandBuffer list()
      {
        return m_list->list();
      }
      vector<VkUploadBlock>& freeableConstants()
      {
        return m_allocatedConstants;
      }
      vector<vk::Pipeline>& oldPipelines()
      {
        return m_oldPipelines;
      }
    };

  }
}