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

#define VK_CHECK_RESULT(value) F_ASSERT(value.result == vk::Result::eSuccess, "Result was not success: \"%s\"", vk::to_string(value.result).c_str())
#define VK_CHECK_RESULT_RAW(value) F_ASSERT(value == vk::Result::eSuccess, "Result was not success: \"%s\"", vk::to_string(value).c_str())


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
          m_aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
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
          m_aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
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
    };

    class VulkanBufferView
    {
    private:
      struct Info
      {
        vk::BufferView view;
        vk::DescriptorBufferInfo bufferInfo;
        vk::DescriptorType type;
      } m;

    public:
      VulkanBufferView()
      {}
      VulkanBufferView(vk::BufferView view, vk::DescriptorType type)
        : m{ view , {}, type}
      {}
      VulkanBufferView(vk::DescriptorBufferInfo view, vk::DescriptorType type)
        : m{ {} , view, type}
      {}
      Info& native()
      {
        return m;
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
        VkUploadBlock block;
      } m;

    public:
      VulkanDynamicBufferView()
      {}
      VulkanDynamicBufferView(vk::Buffer buffer, vk::BufferView view, VkUploadBlock block)
        : m{ buffer, view, {}, vk::DescriptorType::eUniformTexelBuffer, block }
      {}
      VulkanDynamicBufferView(vk::Buffer buffer, vk::DescriptorBufferInfo view, VkUploadBlock block)
        : m{ buffer, {}, view, vk::DescriptorType::eStorageBuffer, block }
      {}
      Info& native()
      {
        return m;
      }
/*
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
      */
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
      GraphicsPipelineDescriptor m_gfxDesc;
      ComputePipelineDescriptor m_computeDesc;

      WatchFile vs;
      WatchFile ds;
      WatchFile hs;
      WatchFile gs;
      WatchFile ps;

      WatchFile cs;
      bool needsUpdating()
      {
        return vs.updated() || ps.updated() || ds.updated() || hs.updated() || gs.updated();
      }

      void updated()
      {
        vs.react();
        ds.react();
        hs.react();
        gs.react();
        ps.react();
      }

      VulkanPipeline() {}

      VulkanPipeline(vk::DescriptorSetLayout descriptorSetLayout, vk::PipelineLayout pipelineLayout, GraphicsPipelineDescriptor gfxDesc)
        : m_hasPipeline(false)
        , m_descriptorSetLayout(descriptorSetLayout)
        , m_pipelineLayout(pipelineLayout)
        , m_gfxDesc(gfxDesc)
      {}

      VulkanPipeline(vk::DescriptorSetLayout descriptorSetLayout, vk::PipelineLayout pipelineLayout, ComputePipelineDescriptor computeDesc)
        : m_hasPipeline(false)
        , m_descriptorSetLayout(descriptorSetLayout)
        , m_pipelineLayout(pipelineLayout)
        , m_computeDesc(computeDesc)
      {}

      VulkanPipeline(vk::Pipeline pipeline, vk::DescriptorSetLayout descriptorSetLayout, vk::PipelineLayout pipelineLayout, ComputePipelineDescriptor computeDesc)
        : m_pipeline(pipeline)
        , m_hasPipeline(true)
        , m_descriptorSetLayout(descriptorSetLayout)
        , m_pipelineLayout(pipelineLayout)
        , m_computeDesc(computeDesc)
      {}
    };

    class VulkanRenderpass
    {
    private:
      vk::RenderPass m_renderpass = nullptr;
      bool is_valid = false;
      size_t m_activeHash = 0;
    public:

      VulkanRenderpass()
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

        auto result = device.createBuffer(natBufferInfo);
        if (result.result == vk::Result::eSuccess)
        {
          m_buffer = result.value;
        }

        vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(m_buffer);
        auto memProp = physDevice.getMemoryProperties();
        auto searchProperties = getMemoryProperties(bufDesc.desc.usage);
        auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
        F_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?");

        vk::MemoryAllocateInfo allocInfo;

        allocInfo = vk::MemoryAllocateInfo()
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        auto allocRes = device.allocateMemory(allocInfo);
        VK_CHECK_RESULT(allocRes);
        m_memory = allocRes.value;
        device.bindBufferMemory(m_buffer, m_memory, 0);
        // we should have cpu UploadBuffer created above and bound, lets map it.

        auto mapResult = device.mapMemory(m_memory, 0, allocationSize*allocationCount);
        VK_CHECK_RESULT(mapResult);

        data = reinterpret_cast<uint8_t*>(mapResult.value);
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

    class VulkanConstantUploadHeap
    {
      FixedSizeAllocator allocator;
      unsigned fixedSize = 1;
      unsigned size = 1;
      vk::Buffer m_buffer;
      vk::DeviceMemory m_memory;
      vk::Device m_device;

      uint8_t* data = nullptr;
    public:
      VulkanConstantUploadHeap() : allocator(1, 1) {}
      VulkanConstantUploadHeap(vk::Device device
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
        F_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?");

        vk::MemoryAllocateInfo allocInfo;

        allocInfo = vk::MemoryAllocateInfo()
          .setAllocationSize(bufDesc.desc.width)
          .setMemoryTypeIndex(index);

        auto allocRes = device.allocateMemory(allocInfo);
        VK_CHECK_RESULT(allocRes);
        m_memory = allocRes.value;
        device.bindBufferMemory(m_buffer, m_memory, 0);
        // we should have cpu UploadBuffer created above and bound, lets map it.

        auto mapResult = device.mapMemory(m_memory, 0, allocationSize*allocationCount);
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
        auto dip = allocator.allocate(bytes);
        F_ASSERT(dip.offset != -1, "No space left, make bigger VulkanUploadHeap :) %d", size);
        return VkUploadBlock{ data, m_buffer,  dip };
      }

      void release(VkUploadBlock desc)
      {
        allocator.release(desc.block);
      }
    };

    class VulkanDescriptorPool
    {
      vk::DescriptorPool pool;
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
          vk::ArrayProxy<const vk::DescriptorSet> csets(sets.size(), sets.data());
          device.freeDescriptorSets(pool, csets);
        }
      }
      vk::DescriptorPool native()
      {
        return pool;
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
      HandleVector<VulkanTextureView> texSRV;
      HandleVector<VulkanTextureView> texUAV;
      HandleVector<VulkanTextureView> texRTV;
      HandleVector<VulkanTextureView> texDSV;
      HandleVector<VulkanPipeline> pipelines;
      HandleVector<VulkanRenderpass> renderpasses;
      HandleVector<VulkanHeap> heaps;
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
      VulkanDescriptorPool m_descriptors;
      vector<vk::DescriptorSet> m_allocatedSets;
      vector<VulkanConstantBuffer> m_constants;
      vector<vk::Pipeline> m_oldPipelines;

    public:
      VulkanCommandBuffer(std::shared_ptr<VulkanCommandList> list, VulkanDescriptorPool descriptors)
        : m_list(list), m_descriptors(descriptors)
      {}
    private:
      void handleBinding(VulkanDevice* device, vk::CommandBuffer buffer, gfxpacket::ResourceBinding& packet, ResourceHandle pipeline);
      void addCommands(VulkanDevice* device, vk::CommandBuffer buffer, backend::CommandBuffer& list, BarrierSolver& solver);
      void handleRenderpass(VulkanDevice* device, gfxpacket::RenderPassBegin& renderpasspacket);
      void preprocess(VulkanDevice* device, backend::CommandBuffer& list);
    public:
      void fillWith(std::shared_ptr<prototypes::DeviceImpl>, backend::CommandBuffer&, BarrierSolver& solver) override;

      vk::CommandBuffer list()
      {
        return m_list->list();
      }

      vector<vk::DescriptorSet>& freeableDescriptors()
      {
        return m_allocatedSets;
      }
      vector<VulkanConstantBuffer>& freeableConstants()
      {
        return m_constants;
      }
      vector<vk::Pipeline>& oldPipelines()
      {
        return m_oldPipelines;
      }
    };

  }
}