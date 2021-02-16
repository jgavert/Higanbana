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
#include "higanbana/graphics/common/pipeline_descriptor.hpp"
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

    class VulkanTimelineSemaphore : public TimelineSemaphoreImpl
    {
      std::shared_ptr<vk::Semaphore> timelineSemaphore;
    public:
      VulkanTimelineSemaphore(std::shared_ptr<vk::Semaphore> tlsema)
        : timelineSemaphore(tlsema)
      {}

      vk::Semaphore native()
      {
        return *timelineSemaphore;
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

    struct VkUploadBlockGPU
    {
      uint8_t* m_data;
      vk::Buffer m_cpuBuffer;
      vk::Buffer m_gpuBuffer;
      RangeBlock block;
      int alignOffset;

      uint8_t* data()
      {
        return m_data + block.offset + alignOffset;
      }

      vk::Buffer bufferCPU()
      {
        return m_cpuBuffer;
      }
      vk::Buffer bufferGPU()
      {
        return m_gpuBuffer;
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
      VulkanDynamicBufferView(vk::Buffer buffer, vk::BufferView texelView, vk::DescriptorBufferInfo view, VkUploadBlock block, vk::IndexType indextype)
        : m{ buffer, texelView, view, vk::DescriptorType::eStorageBuffer, indextype, block, 0 }
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
        b.block.size = roundUpMultiplePowerOf2(bytes, alignment);
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

      vk::MemoryAllocateFlagsInfo flags = vk::MemoryAllocateFlagsInfo().setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
        allocInfo = vk::MemoryAllocateInfo()
          .setPNext(&flags)
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        auto allocRes = device.allocateMemory(allocInfo);
        VK_CHECK_RESULT(allocRes);
        m_memory = allocRes.value;
        auto allocRes2 = device.bindBufferMemory(m_buffer, m_memory, 0);
        VK_CHECK_RESULT_RAW(allocRes2);
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
        return allocator.findLargestAllocation();
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
    public:
      enum class Mode {
        CpuOnly,
        CpuGpu
      };
    private:
      HeapAllocator allocator;
      vk::Buffer m_cpuBuffer;
      vk::DeviceMemory m_cpuMemory;
      vk::Buffer m_gpuBuffer;
      vk::DeviceMemory m_gpuMemory;
      vk::Device m_device;
      Mode m_mode;
      unsigned minUniformBufferAlignment;

      uint8_t* data = nullptr;
      std::mutex m_allocatorBlocker;
    public:
      VulkanConstantUploadHeap() : allocator(1, 1) {}
      VulkanConstantUploadHeap(vk::Device device
                     , vk::PhysicalDevice physDevice
                     , unsigned memorySize
                     , Mode mode
                     , unsigned unaccessibleMemoryAtEnd = 0)
        : allocator(memorySize-unaccessibleMemoryAtEnd, physDevice.getProperties().limits.minUniformBufferOffsetAlignment)
        , m_device(device)
        , minUniformBufferAlignment(physDevice.getProperties().limits.minUniformBufferOffsetAlignment)
        , m_mode(mode)
      {
        auto bufDesc = ResourceDescriptor()
          .setWidth(memorySize)
          .setFormat(FormatType::Uint8)
          .setUsage(ResourceUsage::Upload)
          .setName("DynUpload");

        auto natBufferInfo = fillBufferInfo(bufDesc);
        vk::BufferUsageFlags vkUsage = vk::BufferUsageFlagBits::eUniformBuffer;
        if (m_mode == Mode::CpuGpu)
          vkUsage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc;
        natBufferInfo = natBufferInfo.setUsage(vkUsage);

        auto result = device.createBuffer(natBufferInfo);
        if (result.result == vk::Result::eSuccess)
        {
          m_cpuBuffer = result.value;
        }

        vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(m_cpuBuffer);
        auto memProp = physDevice.getMemoryProperties();
        auto searchProperties = getMemoryProperties(bufDesc.desc.usage);
        auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
        HIGAN_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?");

        vk::MemoryAllocateInfo allocInfo;

        vk::MemoryAllocateFlagsInfo flags = vk::MemoryAllocateFlagsInfo().setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
        allocInfo = vk::MemoryAllocateInfo()
          .setPNext(&flags)
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        auto allocRes = device.allocateMemory(allocInfo);
        VK_CHECK_RESULT(allocRes);
        m_cpuMemory = allocRes.value;
        auto rr = device.bindBufferMemory(m_cpuBuffer, m_cpuMemory, 0);
        VK_CHECK_RESULT_RAW(rr);
        // we should have cpu UploadBuffer created above and bound, lets map it.

        auto mapResult = device.mapMemory(m_cpuMemory, 0, memorySize);
        VK_CHECK_RESULT(mapResult);

        data = reinterpret_cast<uint8_t*>(mapResult.value);
        if (m_mode == Mode::CpuGpu) {
          bufDesc = bufDesc.setUsage(ResourceUsage::GpuReadOnly);
          natBufferInfo = fillBufferInfo(bufDesc);
          natBufferInfo = natBufferInfo.setUsage(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);
          auto result2 = device.createBuffer(natBufferInfo);
          if (result2.result == vk::Result::eSuccess)
          {
            m_gpuBuffer = result2.value;
          }
          requirements = device.getBufferMemoryRequirements(m_gpuBuffer);
          auto memProp = physDevice.getMemoryProperties();
          auto searchProperties = getMemoryProperties(bufDesc.desc.usage);
          auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
          HIGAN_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?");

          allocInfo = vk::MemoryAllocateInfo()
            .setAllocationSize(bufDesc.desc.width)
            .setMemoryTypeIndex(index);

          auto allocRes2 = device.allocateMemory(allocInfo);
          VK_CHECK_RESULT(allocRes2);
          m_gpuMemory = allocRes2.value;
          VK_CHECK_RESULT_RAW(device.bindBufferMemory(m_gpuBuffer, m_gpuMemory, 0));
        }
      }

      ~VulkanConstantUploadHeap()
      {
        m_device.destroyBuffer(m_cpuBuffer);
        m_device.freeMemory(m_cpuMemory);
        if (m_mode == Mode::CpuGpu) {
          m_device.destroyBuffer(m_gpuBuffer);
          m_device.freeMemory(m_gpuMemory);
        }
      }

      VkUploadBlockGPU allocate(size_t bytes)
      {
        std::lock_guard<std::mutex> guard(m_allocatorBlocker);
        auto dip = allocator.allocate(bytes, minUniformBufferAlignment);
        HIGAN_ASSERT(dip, "No space left, make bigger VulkanUploadHeap :) %d", allocator.max_size());
        return VkUploadBlockGPU{ data, m_cpuBuffer, m_gpuBuffer,  dip.value() };
      }

      void release(VkUploadBlockGPU desc)
      {
        std::lock_guard<std::mutex> guard(m_allocatorBlocker);
        allocator.free(desc.block);
      }

      void releaseRange(vector<VkUploadBlockGPU>& descs)
      {
        std::lock_guard<std::mutex> guard(m_allocatorBlocker);
        for (auto&& desc : descs)
          allocator.free(desc.block);
      }

      vk::Buffer buffer()
      {
        if (m_mode == Mode::CpuGpu)
          return m_gpuBuffer;
        return m_cpuBuffer;
      }

      unsigned allocationAlignment() const noexcept
      {
        return minUniformBufferAlignment;
      }

      size_t size() const noexcept
      {
        return allocator.findLargestAllocation();
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
        b.block.size = roundUpMultiplePowerOf2(bytes, alignment);
        return b;
      }
    };
    class VkUploadLinearAllocatorGPU
    {
      LinearAllocator allocator;
      VkUploadBlockGPU block;

    public:
      VkUploadLinearAllocatorGPU() {}
      VkUploadLinearAllocatorGPU(VkUploadBlockGPU block)
        : allocator(block.size())
        , block(block)
      {
      }

      VkUploadBlockGPU allocate(size_t bytes, size_t alignment)
      {
        auto offset = allocator.allocate(bytes, alignment);
        if (offset < 0)
          return VkUploadBlockGPU{ nullptr, vk::Buffer{}, vk::Buffer{}, RangeBlock{} };

        VkUploadBlockGPU b = block;
        b.block.offset += offset;
        b.block.size = roundUpMultiplePowerOf2(bytes, alignment);
        return b;
      }
    };

    struct VkReadbackBlock
    {
      size_t offset;
      size_t m_size;

      size_t size() const
      {
        return m_size;
      }
      explicit operator bool() const
      {
        return size() > 0;
      }
    };

    class VulkanReadbackHeap
    {
      LinearAllocator allocator;
      vk::Buffer m_buffer;
      std::shared_ptr<vk::DeviceMemory> m_memory;
      unsigned fixedSize = 1;
      unsigned m_size = 1;

      uint8_t* data = nullptr;
      ///D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;

      //D3D12_RANGE range{};
    public:
      VulkanReadbackHeap() : allocator(1) {}
      VulkanReadbackHeap(vk::Device device, vk::PhysicalDevice physDevice, unsigned allocationSize, unsigned allocationCount)
        : allocator(allocationSize * allocationCount)
        , fixedSize(allocationSize)
        , m_size(allocationSize*allocationCount)
      {
        auto bufDesc = ResourceDescriptor()
          .setWidth(m_size)
          .setFormat(FormatType::Uint8)
          .setUsage(ResourceUsage::Readback)
          .setName("timestamp readback");

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

        //vk::MemoryAllocateFlagsInfo flags = vk::MemoryAllocateFlagsInfo().setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
        vk::MemoryAllocateInfo allocInfo;

        allocInfo = vk::MemoryAllocateInfo()
//          .setPNext(&flags)
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        auto allocRes = device.allocateMemory(allocInfo);
        VK_CHECK_RESULT(allocRes);
        auto memory = allocRes.value;
        auto rr = device.bindBufferMemory(m_buffer, memory, 0);
        VK_CHECK_RESULT_RAW(rr);
        //range.End = allocationSize * allocationCount;
        m_memory = std::shared_ptr<vk::DeviceMemory>(new vk::DeviceMemory(memory), [dev = device, buf = m_buffer](vk::DeviceMemory* ptr)
        {
          dev.destroyBuffer(buf);
          dev.freeMemory(*ptr);
          delete ptr;
        });
      }

      VkReadbackBlock allocate(size_t bytes)
      {
        auto dip = allocator.allocate(bytes, 512);
        HIGAN_ASSERT(dip != -1, "No space left, make bigger VulkanReadbackHeap :) %d", bytes);
        return VkReadbackBlock{ static_cast<size_t>(dip),  bytes };
      }

      void reset()
      {
        allocator = LinearAllocator(m_size);
      }

      void map(vk::Device device)
      {
        auto mapResult = device.mapMemory(*m_memory, 0, m_size);
        VK_CHECK_RESULT(mapResult);
        data = reinterpret_cast<uint8_t*>(mapResult.value);
      }

      void unmap(vk::Device device)
      {
        device.unmapMemory(*m_memory);
      }

      higanbana::MemView<uint8_t> getView(VkReadbackBlock block)
      {
        return higanbana::MemView<uint8_t>(data + block.offset, block.m_size);
      }

      vk::Buffer buffer()
      {
        return m_buffer;
      }
      vk::DeviceMemory memory()
      {
        return *m_memory;
      }

      size_t size() const noexcept
      {
        return allocator.size();
      }

      size_t max_size() const noexcept
      {
        return allocator.max_size();
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
      std::vector<vk::DescriptorSet> allocate(vk::Device device, vk::DescriptorSetAllocateInfo allocateInfo)
      {
        std::lock_guard<std::mutex> poolLock(descriptorLock);

        auto res = device.allocateDescriptorSets(allocateInfo
          .setDescriptorPool(pool));
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

    struct VulkanQuery
    {
      unsigned beginIndex;
      unsigned endIndex;
    };

    struct VulkanQueryBracket
    {
      VulkanQuery query;
      std::string name;
    };

    class VulkanQueryPool
    {
      LinearAllocator allocator;
      std::shared_ptr<vk::QueryPool> m_pool;
      int m_size = 0;
      uint64_t m_gpuTicksPerSecond = 0;
    public:
      VulkanQueryPool() : allocator(1) {}
      VulkanQueryPool(vk::Device device
                     , vk::PhysicalDevice physDevice
                     , vk::PhysicalDeviceLimits limits
                     , uint32_t queue_family_index
                     , vk::QueryType type
                     , unsigned counterCount)
        : allocator(counterCount)
        , m_size(counterCount)
      {
        auto res = device.createQueryPool(vk::QueryPoolCreateInfo().setQueryType(vk::QueryType::eTimestamp).setQueryCount(counterCount));
        VK_CHECK_RESULT(res);

        m_pool = std::shared_ptr<vk::QueryPool>(new vk::QueryPool(res.value), [=](vk::QueryPool* ptr)
        {
          device.destroyQueryPool(*ptr);
          delete ptr;
        });

        m_gpuTicksPerSecond = static_cast<uint64_t>(1e9 / limits.timestampPeriod);

        /*
        static int queryHeapCount = 0;

        D3D12_QUERY_HEAP_DESC desc{};
        desc.Type = type;
        desc.Count = counterCount;
        desc.NodeMask = 0;

        HIGANBANA_CHECK_HR(device->CreateQueryHeap(&desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));
        auto name = s2ws("QueryHeap" + std::to_string(queryHeapCount));
        heap->SetName(name.c_str());

        HIGANBANA_CHECK_HR(queue->GetTimestampFrequency(&m_gpuTicksPerSecond));
        */
      }

      uint64_t getGpuTicksPerSecond() const
      {
        return m_gpuTicksPerSecond;
      }

      void reset()
      {
        allocator = LinearAllocator(m_size);
      }

      VulkanQuery allocate()
      {
        auto dip = allocator.allocate(2);
        HIGAN_ASSERT(dip != -1, "Queryheap out of queries.");
        return VulkanQuery{ static_cast<unsigned>(dip),  static_cast<unsigned>(dip + 1) };
      }

      vk::QueryPool native()
      {
        return *m_pool;
      }

      size_t size() const noexcept
      {
        return allocator.size();
      }

      size_t max_size() const noexcept
      {
        return allocator.max_size();
      }

      size_t counterSize()
      {
        return sizeof(uint64_t);
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
      vk::DispatchLoaderDynamic m_dispatch;
      vector<std::shared_ptr<vk::Framebuffer>> m_framebuffers;
      std::shared_ptr<VulkanConstantUploadHeap> m_constants;
      std::shared_ptr<VulkanReadbackHeap> m_readback;
      std::shared_ptr<VulkanQueryPool> m_querypool;
      vector<VkUploadBlockGPU> m_allocatedConstants;
      vector<VulkanQuery> m_queries;
      VkReadbackBlock readback;
      vector<vk::Pipeline> m_oldPipelines;
      vector<vk::DescriptorSet> m_tempSets;
      vk::Buffer m_shaderDebugBuffer;

      ResourceHandle m_boundDescriptorSets[HIGANBANA_USABLE_SHADER_ARGUMENT_SETS]; // yeah, lets keep it at 4 for now...
      ViewResourceHandle m_boundIndexBuffer;
      unsigned m_constantAlignment;

      VkUploadLinearAllocatorGPU m_constantsAllocator;

    public:
      VulkanCommandBuffer(
        std::shared_ptr<VulkanCommandList> list,
        std::shared_ptr<VulkanConstantUploadHeap> constantAllocators,
        std::shared_ptr<VulkanReadbackHeap> readback,
        std::shared_ptr<VulkanQueryPool> queryPool,
        vk::Buffer shaderDebug,
        vk::DispatchLoaderDynamic dispatch)
        : m_list(list),
        m_dispatch(dispatch),
        m_constants(constantAllocators),
        m_readback(readback),
        m_querypool(queryPool),
        m_shaderDebugBuffer(shaderDebug),
        m_constantAlignment(m_constants->allocationAlignment())
      {
        m_querypool->reset();
        m_readback->reset();
      }
    private:
      void handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBindingGraphics& packet, ResourceHandle pipeline);
      void handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBindingCompute& packet, ResourceHandle pipeline);
      void addCommands(VulkanDevice* device, vk::CommandBuffer buffer, MemView<backend::CommandBuffer*>& buffers, BarrierSolver& solver);
      void handleRenderpass(VulkanDevice* device, gfxpacket::RenderPassBegin& renderpasspacket);
      void preprocess(VulkanDevice* device, MemView<backend::CommandBuffer*>& list);
      VkUploadBlockGPU allocateConstants(size_t size);
    public:
      void reserveConstants(size_t expectedTotalBytes) override;
      void fillWith(std::shared_ptr<prototypes::DeviceImpl> device, MemView<backend::CommandBuffer*>& buffers, BarrierSolver& solver) override;
      bool readbackTimestamps(std::shared_ptr<prototypes::DeviceImpl> device, vector<GraphNodeTiming>& nodes) override;

      // dma specifics
      void beginConstantsDmaList(std::shared_ptr<prototypes::DeviceImpl> device) override;
      void addConstants(CommandBufferImpl* list) override;
      void endConstantsDmaList() override;

      vk::CommandBuffer list()
      {
        return m_list->list();
      }
      vector<VkUploadBlockGPU>& freeableConstants()
      {
        return m_allocatedConstants;
      }
      vector<VulkanQuery>& freeableQueries()
      {
        return m_queries;
      }
      vector<vk::Pipeline>& oldPipelines()
      {
        return m_oldPipelines;
      }
    };
  }
}