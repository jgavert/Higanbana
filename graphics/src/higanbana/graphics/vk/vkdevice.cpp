#include "higanbana/graphics/vk/vkdevice.hpp"
#include "higanbana/graphics/vk/util/formats.hpp"
#include "higanbana/graphics/common/graphicssurface.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"
#include "higanbana/graphics/common/shader_arguments_descriptor.hpp"
#include "higanbana/graphics/common/raytracing_descriptors.hpp"
#include "higanbana/graphics/desc/shader_arguments_layout_descriptor.hpp"
#include "higanbana/graphics/desc/device_stats.hpp"
#include "higanbana/graphics/vk/util/pipeline_helpers.hpp"
#include "higanbana/graphics/common/helpers/memory_requirements.hpp"
#include "higanbana/graphics/common/helpers/heap_allocation.hpp"
#include "higanbana/graphics/common/helpers/shared_handle.hpp"
#include "higanbana/graphics/common/heap_descriptor.hpp"
#include <higanbana/core/system/bitpacking.hpp>
#include <higanbana/core/global_debug.hpp>
#include <higanbana/core/profiling/profiling.hpp>
#include <higanbana/core/system/time.hpp>

#include <optional>

namespace higanbana
{
  namespace backend
  {
    int32_t FindProperties(vk::PhysicalDeviceMemoryProperties memprop, uint32_t memoryTypeBits, vk::MemoryPropertyFlags properties)
    {
      for (int32_t i = 0; i < static_cast<int32_t>(memprop.memoryTypeCount); ++i)
      {
        if ((memoryTypeBits & (1 << i)) &&
          ((memprop.memoryTypes[i].propertyFlags & properties) == properties))
          return i;
      }
      return -1;
    }

    vk::BufferCreateInfo fillBufferInfo(ResourceDescriptor descriptor, bool memoryAddressingEnabled, size_t minStructuredBufferAlignment)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto desc = descriptor.desc;
      if (desc.stride == 0)
        desc.stride = 1;
      if (desc.structured) {
        if (desc.stride%minStructuredBufferAlignment != 0) {
          desc.stride += (minStructuredBufferAlignment - desc.stride%minStructuredBufferAlignment);
        }
      }

      auto bufSize = desc.stride*desc.width;
      HIGAN_ASSERT(bufSize != 0, "Cannot create zero sized buffers.");
      vk::BufferCreateInfo info = vk::BufferCreateInfo()
        .setSharingMode(vk::SharingMode::eExclusive);

      vk::BufferUsageFlags usageBits;

      // testing multiple flags
      usageBits = vk::BufferUsageFlagBits::eUniformTexelBuffer;
      usageBits |= vk::BufferUsageFlagBits::eStorageBuffer;
      if (memoryAddressingEnabled)
        usageBits |= vk::BufferUsageFlagBits::eShaderDeviceAddress;

      if (desc.usage == ResourceUsage::GpuRW)
      {
        usageBits |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
      }
      if (desc.indirect)
      {
        usageBits |= vk::BufferUsageFlagBits::eIndirectBuffer;
      }

      if (desc.index)
      {
        usageBits |= vk::BufferUsageFlagBits::eIndexBuffer;
      }

      auto usage = desc.usage;
      if (usage == ResourceUsage::Readback)
      {
        usageBits = usageBits | vk::BufferUsageFlagBits::eTransferDst;
      }
      else if (usage == ResourceUsage::Upload)
      {
        usageBits = usageBits | vk::BufferUsageFlagBits::eTransferSrc;
      }
      else
      {
        usageBits = usageBits | vk::BufferUsageFlagBits::eTransferSrc;
        usageBits = usageBits | vk::BufferUsageFlagBits::eTransferDst;
      }
      info = info.setUsage(usageBits);
      info = info.setSize(bufSize);
      return info;
    }

    vk::ImageCreateInfo fillImageInfo(ResourceDescriptor descriptor)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto desc = descriptor.desc;

      vk::ImageType ivt;
      vk::ImageCreateFlags flags;
      flags |= vk::ImageCreateFlagBits::eMutableFormat; //???
      switch (desc.dimension)
      {
      case FormatDimension::Texture1D:
        ivt = vk::ImageType::e1D;
        break;
      case FormatDimension::Texture2D:
        ivt = vk::ImageType::e2D;
        break;
      case FormatDimension::Texture3D:
        ivt = vk::ImageType::e3D;
        break;
      case FormatDimension::TextureCube:
        ivt = vk::ImageType::e2D;
        flags |= vk::ImageCreateFlagBits::eCubeCompatible;
        break;
      default:
        ivt = vk::ImageType::e2D;
        break;
      }

      int mipLevels = desc.miplevels;
      HIGAN_ASSERT(mipLevels != ResourceDescriptor::AllMips, "shouldn't be wrong");

      vk::ImageUsageFlags usage;
      usage |= vk::ImageUsageFlagBits::eTransferDst;
      usage |= vk::ImageUsageFlagBits::eTransferSrc;
      usage |= vk::ImageUsageFlagBits::eSampled;

      switch (desc.usage)
      {
      case ResourceUsage::GpuReadOnly:
      {
        break;
      }
      case ResourceUsage::GpuRW:
      {
        usage |= vk::ImageUsageFlagBits::eStorage;
        break;
      }
      case ResourceUsage::RenderTarget:
      {
        usage |= vk::ImageUsageFlagBits::eColorAttachment;
        break;
      }
      case ResourceUsage::RenderTargetRW:
      {
        usage |= vk::ImageUsageFlagBits::eColorAttachment;
        usage |= vk::ImageUsageFlagBits::eStorage;
        break;
      }
      case ResourceUsage::DepthStencil:
      {
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        HIGAN_ASSERT(mipLevels == 1, "DepthStencil doesn't support mips");
        break;
      }
      case ResourceUsage::DepthStencilRW:
      {
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        usage |= vk::ImageUsageFlagBits::eStorage;
        HIGAN_ASSERT(mipLevels == 1, "DepthStencil doesn't support mips");
        break;
      }
      case ResourceUsage::Upload:
      {
        usage = vk::ImageUsageFlagBits::eTransferSrc;
        break;
      }
      case ResourceUsage::Readback:
      {
        usage = vk::ImageUsageFlagBits::eTransferDst;
        break;
      }
      default:
        break;
      }

      vk::SampleCountFlagBits sampleFlags = vk::SampleCountFlagBits::e1;

      if (desc.msCount > 32)
      {
        sampleFlags = vk::SampleCountFlagBits::e64;
      }
      else if (desc.msCount > 16)
      {
        sampleFlags = vk::SampleCountFlagBits::e32;
      }
      else if (desc.msCount > 8)
      {
        sampleFlags = vk::SampleCountFlagBits::e16;
      }
      else if (desc.msCount > 4)
      {
        sampleFlags = vk::SampleCountFlagBits::e8;
      }
      else if (desc.msCount > 2)
      {
        sampleFlags = vk::SampleCountFlagBits::e4;
      }
      else if (desc.msCount > 1)
      {
        sampleFlags = vk::SampleCountFlagBits::e2;
      }

      vk::ImageCreateInfo info = vk::ImageCreateInfo()
        .setArrayLayers(desc.arraySize)
        .setExtent(vk::Extent3D()
          .setWidth(desc.width)
          .setHeight(desc.height)
          .setDepth(desc.depth))
        .setFlags(flags)
        .setFormat(formatToVkFormat(desc.format).storage)
        .setImageType(ivt)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setMipLevels(mipLevels)
        .setSamples(sampleFlags)
        .setTiling(vk::ImageTiling::eOptimal) // TODO: hmm
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive); // TODO: hmm

      if (desc.dimension == FormatDimension::TextureCube)
      {
        info = info.setArrayLayers(desc.arraySize * 6);
      }

      return info;
    }

    MemoryPropertySearch getMemoryProperties(ResourceUsage usage)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      MemoryPropertySearch ret{};
      switch (usage)
      {
      case ResourceUsage::Upload:
      {
        ret.optimal = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        ret.def = vk::MemoryPropertyFlagBits::eHostVisible;
        break;
      }
      case ResourceUsage::Readback:
      {
        ret.optimal = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached;
        ret.def = vk::MemoryPropertyFlagBits::eHostVisible;
        break;
      }
      case ResourceUsage::GpuReadOnly:
      case ResourceUsage::GpuRW:
      case ResourceUsage::RenderTarget:
      case ResourceUsage::DepthStencil:
      case ResourceUsage::RenderTargetRW:
      case ResourceUsage::DepthStencilRW:
      default:
      {
        ret.optimal = vk::MemoryPropertyFlagBits::eDeviceLocal;
        ret.def = vk::MemoryPropertyFlagBits::eDeviceLocal;
        break;
      }
      }
      return ret;
    }

    void printMemoryTypeInfos(vk::PhysicalDeviceMemoryProperties prop)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto checkFlagSet = [](vk::MemoryType& type, vk::MemoryPropertyFlagBits flag)
      {
        return (type.propertyFlags & flag) == flag;
      };
      HIGAN_ILOG("higanbana/graphics/Memory", "heapCount %u memTypeCount %u", prop.memoryHeapCount, prop.memoryTypeCount);
      for (int i = 0; i < static_cast<int>(prop.memoryTypeCount); ++i)
      {
        auto memType = prop.memoryTypes[i];
        HIGAN_ILOG("higanbana/graphics/Memory", "propertyFlags %u", memType.propertyFlags.operator unsigned int());
        if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
          HIGAN_ILOG("higanbana/graphics/Memory", "heap %u type %u was eDeviceLocal", memType.heapIndex, i);
          if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
          {
            HIGAN_ILOG("higanbana/graphics/Memory", "heap %u type %u was eHostVisible", memType.heapIndex, i);
            if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCoherent))
              HIGAN_ILOG("higanbana/graphics/Memory", "heap %u type %u was eHostCoherent", memType.heapIndex, i);
            if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
              HIGAN_ILOG("higanbana/graphics/Memory", "heap %u type %u was eHostCached", memType.heapIndex, i);
          }
        }
        else if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
        {
          HIGAN_ILOG("higanbana/graphics/Memory", "heap %u type %u was eHostVisible", memType.heapIndex, i);
          if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCoherent))
            HIGAN_ILOG("higanbana/graphics/Memory", "heap %u type %u was eHostCoherent", memType.heapIndex, i);
          if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
          {
            HIGAN_ILOG("higanbana/graphics/Memory", "heap %u type %u was eHostCached", memType.heapIndex, i);
          }
        }
      }
    }

    vk::MemoryAllocateFlags VulkanDevice::memoryAddressingFlags() const {
      if (m_memoryAddressingEnabled)
        return vk::MemoryAllocateFlagBits::eDeviceAddress;
      return vk::MemoryAllocateFlags(0);
    }

    VulkanDevice::VulkanDevice(
      vk::Device device,
      vk::PhysicalDevice physDev,
      vk::DispatchLoaderDynamic dynamicDispatch,
      FileSystem& fs,
      std::vector<vk::QueueFamilyProperties> queues,
      GpuInfo info,
      bool debugLayer,
      bool memoryAddressingEnabled)
      : m_device(device)
      , m_physDevice(physDev)
      , m_limits(physDev.getProperties().limits)
      , m_dynamicDispatch(dynamicDispatch)
      , m_memoryAddressingEnabled(memoryAddressingEnabled) 
      , m_debugLayer(debugLayer)
      , m_queues(queues)
      , m_singleQueue(false)
      , m_computeQueues(false)
      , m_dmaQueues(false)
      , m_graphicQueues(false)
      , m_info(info)
      , m_shaders(fs, std::shared_ptr<ShaderCompiler>(new DXCompiler(fs)), "/shader_binaries", ShaderBinaryType::SPIRV, info.forceCompileShaders)
      , m_freeQueueIndexes({})
      //, m_seqTracker(std::make_shared<SequenceTracker>())
      , m_dynamicUpload(std::make_shared<VulkanUploadHeap>(device, physDev, memoryAddressingFlags(), HIGANBANA_UPLOAD_MEMORY_AMOUNT)) // TODO: implement dynamically adjusted
      , m_constantAllocators(std::make_shared<VulkanConstantUploadHeap>(device, physDev, memoryAddressingFlags(), HIGANBANA_CONSTANT_BUFFER_AMOUNT, info.gpuConstants ? VulkanConstantUploadHeap::Mode::CpuGpu : VulkanConstantUploadHeap::Mode::CpuOnly, 1024)) // TODO: implement dynamically adjusted
      , m_descriptorSetsInUse(0)
//      , m_trash(std::make_shared<Garbage>())
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto features2 = physDev.getFeatures2<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan12Features>();
      auto& features12 = features2.get<vk::PhysicalDeviceVulkan12Features>();
      // try to figure out unique queues, abort or something when finding unsupported count.
      // universal
      // graphics+compute
      // compute
      // dma
      size_t totalQueues = 0;
      for (int k = 0; k < static_cast<int>(m_queues.size()); ++k)
      {
        constexpr auto GfxQ = static_cast<uint32_t>(VK_QUEUE_GRAPHICS_BIT);
        constexpr auto CpQ = static_cast<uint32_t>(VK_QUEUE_COMPUTE_BIT);
        constexpr auto DMAQ = static_cast<uint32_t>(VK_QUEUE_TRANSFER_BIT);
        auto&& it = m_queues[k];
        auto current = static_cast<uint32_t>(it.queueFlags);
        if ((current & (GfxQ | CpQ | DMAQ)) == GfxQ + CpQ + DMAQ)
        {
          for (uint32_t i = 0; i < it.queueCount; ++i)
          {
            m_freeQueueIndexes.universal.push_back(i);
          }
          m_freeQueueIndexes.universalIndex = k;
        }
        else if ((current & (GfxQ | CpQ)) == GfxQ + CpQ)
        {
          for (uint32_t i = 0; i < it.queueCount; ++i)
          {
            m_freeQueueIndexes.graphics.push_back(i);
          }
          m_freeQueueIndexes.graphicsIndex = k;
        }
        else if ((current & CpQ) == CpQ)
        {
          for (uint32_t i = 0; i < it.queueCount; ++i)
          {
            m_freeQueueIndexes.compute.push_back(i);
          }
          m_freeQueueIndexes.computeIndex = k;
        }
        else if ((current & DMAQ) == DMAQ)
        {
          for (uint32_t i = 0; i < it.queueCount; ++i)
          {
            m_freeQueueIndexes.dma.push_back(i);
          }
          m_freeQueueIndexes.dmaIndex = k;
        }
        totalQueues += it.queueCount;
      }
      if (totalQueues == 0)
      {
        HIGAN_ERROR("wtf, not sane device.");
      }
      else if (totalQueues == 1)
      {
        // single queue =_=, IIIINNNTTTEEEELLL
        m_singleQueue = true;
        // lets just fetch it and copy it for those who need it.
        //auto que = m_device->getQueue(0, 0); // TODO: 0 index is wrong.
        //m_internalUniversalQueue = m_device.getQueue(0, 0);
      }
      if (m_freeQueueIndexes.universal.size() > 0
        && m_freeQueueIndexes.graphics.size() > 0)
      {
        HIGAN_ERROR("abort mission. Too many variations of queues.");
      }

      m_computeQueues = !m_freeQueueIndexes.compute.empty();
      m_dmaQueues = !m_freeQueueIndexes.dma.empty();
      m_graphicQueues = !m_freeQueueIndexes.graphics.empty();

      // find graphics, compute, copy queues
      if (m_singleQueue)
      {
        m_mainQueue = m_device.getQueue(0, 0);
        m_mainQueueIndex = 0;
      }
      else
      {
        if (!m_freeQueueIndexes.graphics.empty())
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.graphicsIndex;
          uint32_t queueId = m_freeQueueIndexes.graphics.back();
          m_freeQueueIndexes.graphics.pop_back();
          m_mainQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_mainQueueIndex = queueFamilyIndex;
        }
        else if (!m_freeQueueIndexes.universal.empty())
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.universalIndex;
          uint32_t queueId = m_freeQueueIndexes.universal.back();
          m_freeQueueIndexes.universal.pop_back();
          m_mainQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_mainQueueIndex = queueFamilyIndex;
        }
        // compute
        if (!m_freeQueueIndexes.compute.empty())
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.computeIndex;
          uint32_t queueId = m_freeQueueIndexes.compute.back();
          m_freeQueueIndexes.compute.pop_back();
          m_computeQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_computeQueueIndex = queueFamilyIndex;
        }
        else if (!m_freeQueueIndexes.universal.empty())
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.universalIndex;
          uint32_t queueId = m_freeQueueIndexes.universal.back();
          m_freeQueueIndexes.universal.pop_back();
          m_computeQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_computeQueueIndex = queueFamilyIndex;
        }

        // copy
        if (!m_freeQueueIndexes.dma.empty())
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.dmaIndex;
          uint32_t queueId = m_freeQueueIndexes.dma.back();
          m_freeQueueIndexes.dma.pop_back();
          m_copyQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_copyQueueIndex = queueFamilyIndex;
        }
        else if (!m_freeQueueIndexes.universal.empty())
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.universalIndex;
          uint32_t queueId = m_freeQueueIndexes.universal.back();
          m_freeQueueIndexes.universal.pop_back();
          m_copyQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_copyQueueIndex = queueFamilyIndex;
        }
      }

      m_semaphores = Rabbitpool2<VulkanSemaphore>([device]()
      {
        auto fence = device.createSemaphore(vk::SemaphoreCreateInfo());
        VK_CHECK_RESULT(fence);
        return VulkanSemaphore(std::shared_ptr<vk::Semaphore>(new vk::Semaphore(fence.value), [=](vk::Semaphore* ptr)
        {
          device.destroySemaphore(*ptr);
          delete ptr;
        }));
      });
      m_fences = Rabbitpool2<VulkanFence>([device]()
      {
        auto fence = device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
        VK_CHECK_RESULT(fence);
        return VulkanFence(std::shared_ptr<vk::Fence>(new vk::Fence(fence.value), [=](vk::Fence* ptr)
        {
          device.destroyFence(*ptr);
          delete ptr;
        }));
      });

      // If we didn't have the queues, we don't really have anything for the lists either.
      m_copyQueueIndex = m_copyQueueIndex != -1 ? m_copyQueueIndex : m_mainQueueIndex;
      m_computeQueueIndex = m_computeQueueIndex != -1 ? m_computeQueueIndex : m_mainQueueIndex;

      // check timestamp query support.
      HIGAN_ASSERT(m_queues[m_mainQueueIndex].timestampValidBits != 0, "doesn't support graphics timestamps");
      HIGAN_ASSERT(m_queues[m_copyQueueIndex].timestampValidBits != 0, "doesn't support dma timestamps");
      HIGAN_ASSERT(m_queues[m_computeQueueIndex].timestampValidBits != 0, "doesn't support compute timestamps");

      m_copyListPool = Rabbitpool2<VulkanCommandList>([&]() {return createCommandBuffer(m_copyQueueIndex); });
      m_computeListPool = Rabbitpool2<VulkanCommandList>([&]() {return createCommandBuffer(m_computeQueueIndex); });
      m_graphicsListPool = Rabbitpool2<VulkanCommandList>([&]() {return createCommandBuffer(m_mainQueueIndex); });

      m_queryPoolPool = Rabbitpool2<VulkanQueryPool>([&]()
      {
        return createGraphicsQueryPool(1024);
      });

      m_computeQueryPoolPool = Rabbitpool2<VulkanQueryPool>([&]()
      {
        return createComputeQueryPool(1024);
      });

      m_dmaQueryPoolPool = Rabbitpool2<VulkanQueryPool>([&]()
      {
        return createDMAQueryPool(512);
      });

      m_readbackPool = Rabbitpool2<VulkanReadbackHeap>([&]()
      {
        return createReadback(256 * 10, 1024); // maybe 10 megs of readback?
      });


#if defined(HIGANBANA_PLATFORM_WINDOWS)
      bool mainQueuePresents = m_physDevice.getWin32PresentationSupportKHR(m_mainQueueIndex);
      bool computePresents = m_computeQueueIndex >= 0 ? m_physDevice.getWin32PresentationSupportKHR(m_computeQueueIndex) : false;
#else
      bool mainQueuePresents = true; // todo: add proper detection to linux
      bool computePresents = false;
#endif

      //auto memProp = m_physDevice.getMemoryProperties();
      //printMemoryTypeInfos(memProp);
      /* if VK_KHR_display is a things, use the below. For now, Borderless fullscreen!
      {
          auto displayProperties = m_physDevice.getDisplayPropertiesKHR();
          HIGAN_SLOG("Vulkan", "Trying to query available displays...\n");
          for (auto&& prop : displayProperties)
          {
              HIGAN_LOG("%s Physical Dimensions: %dx%d Resolution: %dx%d %s %s\n",
                  prop.displayName, prop.physicalDimensions.width, prop.physicalDimensions.height,
                  prop.physicalResolution.width, prop.physicalResolution.height,
                  ((prop.planeReorderPossible) ? " Plane Reorder possible " : ""),
                  ((prop.persistentContent) ? " Persistent Content " : ""));
          }
      }*/

      // lets make some immutable samplers for shaders...

      vk::SamplerCreateInfo s0 = vk::SamplerCreateInfo()
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge);
      vk::SamplerCreateInfo s1 = vk::SamplerCreateInfo()
        .setMagFilter(vk::Filter::eNearest)
        .setMinFilter(vk::Filter::eNearest)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge);
      vk::SamplerCreateInfo s2 = vk::SamplerCreateInfo()
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat);
      vk::SamplerCreateInfo s3 = vk::SamplerCreateInfo()
        .setMagFilter(vk::Filter::eNearest)
        .setMinFilter(vk::Filter::eNearest)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat);

      auto sr0 = m_device.createSampler(s0);
      auto sr1 = m_device.createSampler(s1);
      auto sr2 = m_device.createSampler(s2);
      auto sr3 = m_device.createSampler(s3);
      VK_CHECK_RESULT(sr0);
      VK_CHECK_RESULT(sr1);
      VK_CHECK_RESULT(sr2);
      VK_CHECK_RESULT(sr3);
      m_samplers.m_bilinearSampler = sr0.value;
      m_samplers.m_pointSampler = sr1.value;
      m_samplers.m_bilinearSamplerWrap = sr2.value;
      m_samplers.m_pointSamplerWrap = sr3.value;

      /////////////////// DESCRIPTORS ////////////////////////////////
      // aim for total 512k single descriptors
      constexpr const int totalDescriptorsAvailable = 1024*512;
      constexpr const int samplers = 0;
      constexpr const int dynamicUniform = 0;
      constexpr const int sampledImages = totalDescriptorsAvailable / 2;
      constexpr const int storageImages = 10000;
      constexpr const int storageBuffers = 10000;
      constexpr const int texelBuffers = 10000;
      constexpr const int storageTexelBuffers = 10000;
      constexpr const int totalUniqueDescriptorSets = 10000;
      vector<vk::DescriptorPoolSize> dps;
      //dps.push_back(vk::DescriptorPoolSize().setDescriptorCount(dynamicUniform).setType(vk::DescriptorType::eUniformBufferDynamic));
      //dps.push_back(vk::DescriptorPoolSize().setDescriptorCount(samplers).setType(vk::DescriptorType::eSampler));
      // actual possible user values
      dps.push_back(vk::DescriptorPoolSize().setDescriptorCount(sampledImages).setType(vk::DescriptorType::eSampledImage));
      dps.push_back(vk::DescriptorPoolSize().setDescriptorCount(storageImages).setType(vk::DescriptorType::eStorageImage));
      dps.push_back(vk::DescriptorPoolSize().setDescriptorCount(storageBuffers).setType(vk::DescriptorType::eStorageBuffer));
      dps.push_back(vk::DescriptorPoolSize().setDescriptorCount(texelBuffers).setType(vk::DescriptorType::eUniformTexelBuffer));
      dps.push_back(vk::DescriptorPoolSize().setDescriptorCount(storageTexelBuffers).setType(vk::DescriptorType::eStorageTexelBuffer));


      auto poolRes = m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo()
        .setMaxSets(totalUniqueDescriptorSets)
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT)
        .setPoolSizeCount(dps.size())
        .setPPoolSizes(dps.data()));

      m_maxDescriptorSets = totalUniqueDescriptorSets;

      VK_CHECK_RESULT(poolRes);

      m_descriptors = std::make_shared<VulkanDescriptorPool>(poolRes.value);

      {
        auto bdesc = ResourceDescriptor()
          .setName("Shader debug print buffer")
          .setFormat(FormatType::Raw32)
          .setWidth(HIGANBANA_SHADER_DEBUG_WIDTH)
          .setUsage(ResourceUsage::GpuRW)
          .allowSimultaneousAccess();
        
        auto vkdesc = fillBufferInfo(bdesc, m_memoryAddressingEnabled, m_limits.minStorageBufferOffsetAlignment);
        auto buffer = m_device.createBuffer(vkdesc);
        VK_CHECK_RESULT(buffer);
        
        vk::MemoryAllocateFlagsInfo flags = vk::MemoryAllocateFlagsInfo().setFlags(memoryAddressingFlags());
        vk::MemoryDedicatedAllocateInfoKHR dediInfo;
        dediInfo = dediInfo.setBuffer(buffer.value).setPNext(&flags);
        auto chain = m_device.getBufferMemoryRequirements2KHR<vk::MemoryRequirements2, vk::MemoryDedicatedRequirementsKHR>(vk::BufferMemoryRequirementsInfo2KHR().setBuffer(buffer.value), m_dynamicDispatch);

        auto dedReq = chain.get<vk::MemoryDedicatedRequirementsKHR>();

        auto bufMemReq = chain.get<vk::MemoryRequirements2>();

        auto memProp = m_physDevice.getMemoryProperties();
        auto searchProperties = getMemoryProperties(bdesc.desc.usage);
        auto index = FindProperties(memProp, bufMemReq.memoryRequirements.memoryTypeBits, searchProperties.optimal);

        auto res = m_device.allocateMemory(vk::MemoryAllocateInfo().setPNext(&dediInfo).setAllocationSize(bufMemReq.memoryRequirements.size).setMemoryTypeIndex(index));
        VK_CHECK_RESULT(res);

        VK_CHECK_RESULT_RAW(m_device.bindBufferMemory(buffer.value, res.value, 0));

        m_shaderDebugBuffer = VulkanBuffer(buffer.value, res.value);
      }

      // constant+static samplers descriptorlayout
      {
        auto bindings = defaultSetLayoutBindings(vk::ShaderStageFlagBits::eAll);
        vk::DescriptorSetLayoutCreateInfo dslinfo = vk::DescriptorSetLayoutCreateInfo()
          .setBindingCount(static_cast<uint32_t>(bindings.size()))
          .setPBindings(bindings.data());

        auto setlayout = m_device.createDescriptorSetLayout(dslinfo);
        VK_CHECK_RESULT(setlayout);
        m_defaultDescriptorLayout = VulkanShaderArgumentsLayout(setlayout.value);
      }

      // null resources
      for (auto&& format : nullFormats) {
        auto res = m_device.createBuffer(fillBufferInfo(ResourceDescriptor()
          .setFormat(format)
          .setWidth(1), m_memoryAddressingEnabled, m_limits.minStorageBufferOffsetAlignment));
        VK_CHECK_RESULT(res);
        nullBuffers(format).buffer = res.value;

        auto imDesc = ResourceDescriptor()
          .setFormat(format)
          .setSize(uint3(1,1,1))
          .setUsage(ResourceUsage::GpuRW)
          .setDimension(FormatDimension::TextureCube);
        auto image = m_device.createImage(fillImageInfo(imDesc));
        VK_CHECK_RESULT(image);
        nullImages(format).nullImageCube = image.value;
        {
          imDesc = imDesc.setDimension(FormatDimension::Texture1D);
          image = m_device.createImage(fillImageInfo(imDesc));
          VK_CHECK_RESULT(image);
          nullImages(format).nullImage1d = image.value;
        }
        {
          imDesc = imDesc.setDimension(FormatDimension::Texture3D);
          image = m_device.createImage(fillImageInfo(imDesc));
          VK_CHECK_RESULT(image);
          nullImages(format).nullImage3d = image.value;
        }
      }
      size_t allocateMemoryBuffers = 0;
      size_t allocateMemoryImages = 0;
      vector<vk::MemoryRequirements> reqs;
      for (auto&& format : nullFormats) {
        reqs.push_back(m_device.getBufferMemoryRequirements(nullBuffers(format).buffer));
        allocateMemoryBuffers += roundUpMultiple(reqs.back().size, reqs.back().alignment);
        reqs.push_back(m_device.getImageMemoryRequirements(nullImages(format).nullImageCube));
        allocateMemoryImages += roundUpMultiple(reqs.back().size, reqs.back().alignment);
        reqs.push_back(m_device.getImageMemoryRequirements(nullImages(format).nullImage1d));
        allocateMemoryImages += roundUpMultiple(reqs.back().size, reqs.back().alignment);
        reqs.push_back(m_device.getImageMemoryRequirements(nullImages(format).nullImage3d));
        allocateMemoryImages += roundUpMultiple(reqs.back().size, reqs.back().alignment);
      }
      auto sampleBitsBuffer = reqs[0].memoryTypeBits;
      auto sampleBitsImage = reqs[1].memoryTypeBits;
      vk::MemoryAllocateInfo allocInfo;
      auto memProp = m_physDevice.getMemoryProperties();
      auto searchProperties = getMemoryProperties(ResourceUsage::GpuRW);
      auto index = FindProperties(memProp, sampleBitsBuffer, searchProperties.optimal);
      HIGAN_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?"); // searchProperties.de

      vk::MemoryAllocateFlagsInfo flags = vk::MemoryAllocateFlagsInfo().setFlags(memoryAddressingFlags());
      allocInfo = vk::MemoryAllocateInfo()
        .setPNext(&flags)
        .setAllocationSize(allocateMemoryBuffers)
        .setMemoryTypeIndex(index);

      auto memory = m_device.allocateMemory(allocInfo);
      VK_CHECK_RESULT(memory);
      nullHeapBuffers = memory.value;
      index = FindProperties(memProp, sampleBitsImage, searchProperties.optimal);
      HIGAN_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?"); // searchProperties.de

      allocInfo = vk::MemoryAllocateInfo()
        .setPNext(&flags)
        .setAllocationSize(allocateMemoryImages)
        .setMemoryTypeIndex(index);

      memory = m_device.allocateMemory(allocInfo);
      VK_CHECK_RESULT(memory);
      nullHeapImages = memory.value;
      size_t offsetBuffers = 0;
      size_t offset = 0;
      int reqsIndex = 0;

      for (auto&& format : nullFormats) {
        VK_CHECK_RESULT_RAW(m_device.bindBufferMemory(nullBuffers(format).buffer, nullHeapBuffers, offsetBuffers));
        offsetBuffers += roundUpMultiple(reqs[reqsIndex].size, reqs[reqsIndex].alignment);
        reqsIndex++;
        VK_CHECK_RESULT_RAW(m_device.bindImageMemory(nullImages(format).nullImageCube, nullHeapImages, offset));
        offset += roundUpMultiple(reqs[reqsIndex].size, reqs[reqsIndex].alignment);
        reqsIndex++;
        VK_CHECK_RESULT_RAW(m_device.bindImageMemory(nullImages(format).nullImage1d, nullHeapImages, offset));
        offset += roundUpMultiple(reqs[reqsIndex].size, reqs[reqsIndex].alignment);
        reqsIndex++;
        VK_CHECK_RESULT_RAW(m_device.bindImageMemory(nullImages(format).nullImage3d, nullHeapImages, offset));
        offset += roundUpMultiple(reqs[reqsIndex].size, reqs[reqsIndex].alignment);
        reqsIndex++;
      }

      for (auto&& format : nullFormats) {
        nullBuffers(format).ssbo = vk::DescriptorBufferInfo()
          .setBuffer(nullBuffers(format).buffer)
          .setOffset(0)
          .setRange(VK_WHOLE_SIZE);

        vk::Format nullFormat2 = formatToVkFormat(format).view;
        auto viewRes = m_device.createBufferView(vk::BufferViewCreateInfo()
          .setBuffer(nullBuffers(format).buffer)
          .setFormat(nullFormat2)
          .setOffset(0)
          .setRange(VK_WHOLE_SIZE));
        VK_CHECK_RESULT(viewRes);
        nullBuffers(format).texel = viewRes.value;
      }
      for (auto&& format : nullFormats) {
        // null texture views
        auto& ni = nullImages(format);
        vk::Format nullFormat2 = formatToVkFormat(format).view;
        auto makeView = [&](vk::ImageViewType ivt, vk::Image image, bool isCube = false){
          vk::ImageAspectFlags imgFlags = vk::ImageAspectFlagBits::eColor;
          vk::ImageSubresourceRange subResourceRange = vk::ImageSubresourceRange()
            .setAspectMask(imgFlags)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setLayerCount((isCube ? 6 : 1));
          vk::ComponentMapping cm = vk::ComponentMapping()
            .setA(vk::ComponentSwizzle::eIdentity)
            .setR(vk::ComponentSwizzle::eIdentity)
            .setG(vk::ComponentSwizzle::eIdentity)
            .setB(vk::ComponentSwizzle::eIdentity);

          vk::ImageViewCreateInfo vv = vk::ImageViewCreateInfo()
            .setViewType(ivt)
            .setFormat(nullFormat2)
            .setImage(image)
            .setComponents(cm)
            .setSubresourceRange(subResourceRange);

          auto view = m_device.createImageView(vv);
          VK_CHECK_RESULT(view);
          return view.value;
        };
        ni.oneDim = makeView(vk::ImageViewType::e1D, ni.nullImage1d);
        ni.oneDimArray = makeView(vk::ImageViewType::e1DArray, ni.nullImage1d);
        ni.twoDim = makeView(vk::ImageViewType::e2D, ni.nullImageCube);
        ni.twoDimArray = makeView(vk::ImageViewType::e2DArray, ni.nullImageCube);
        ni.threeDim = makeView(vk::ImageViewType::e3D, ni.nullImage3d);
        ni.cube = makeView(vk::ImageViewType::eCube, ni.nullImageCube, true);
        ni.cubeArray = makeView(vk::ImageViewType::eCubeArray, ni.nullImageCube, true);
        vk::DescriptorImageInfo dii = vk::DescriptorImageInfo().setImageLayout(vk::ImageLayout::eGeneral).setImageView(ni.oneDim);
        ni.desc1D = dii;
        ni.desc1DArray = dii.setImageView(ni.oneDimArray);
        ni.desc2D = dii.setImageView(ni.twoDim);
        ni.desc2DArray = dii.setImageView(ni.twoDimArray);
        ni.desc3D = dii.setImageView(ni.threeDim);
        ni.descCube = dii.setImageView(ni.cube);
        ni.descCubeArray = dii.setImageView(ni.cubeArray);
      }
      // do vulkan command buffers and submit them...
      auto list = createCommandBuffer(m_freeQueueIndexes.universalIndex);
      VK_CHECK_RESULT_RAW(list.list().begin(vk::CommandBufferBeginInfo()));
      vector<vk::ImageMemoryBarrier> barriers;
      for (auto&& format : nullFormats) {
        // null texture views
        auto& ni = nullImages(format);
        vk::ImageMemoryBarrier barr = vk::ImageMemoryBarrier()
          .setImage(ni.nullImage1d)
          .setSubresourceRange(vk::ImageSubresourceRange().setAspectMask(vk::ImageAspectFlagBits::eColor).setLevelCount(1).setLayerCount(1))
          .setOldLayout(vk::ImageLayout::eUndefined)
          .setNewLayout(vk::ImageLayout::eGeneral);
        barriers.push_back(barr);
        barriers.push_back(barr.setImage(ni.nullImage3d));
        barriers.push_back(barr.setImage(ni.nullImageCube));
      }

      list.list().pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(0), {}, {}, barriers);
      VK_CHECK_RESULT_RAW(list.list().end());
      vk::CommandBuffer buffer = list.list();
      auto si = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setPCommandBuffers(&buffer);
      vk::Fence fence;
      auto res = m_mainQueue.submit({si}, fence);
      VK_CHECK_RESULT_RAW(res);
      VK_CHECK_RESULT_RAW(m_device.waitIdle());
    }

    VulkanDevice::~VulkanDevice()
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      VK_CHECK_RESULT_RAW(m_device.waitIdle());

      m_device.destroyBuffer(m_shaderDebugBuffer.native());
      m_device.freeMemory(m_shaderDebugBuffer.sharedMem());
      // Clear all user resources nicely
      // descriptorSets
      for (auto&& arg : m_allRes.shaArgs.view()) {
        if (arg) {
          auto set = arg.native();
          m_descriptors->freeSets(m_device, makeMemView(set));
        }
      }
      // descriptors
      for (auto&& view : m_allRes.dynBuf.view()) {
        if (view) {
          auto nat = view.native();
          m_device.destroyBufferView(nat.texelView);
        }
      }
      for (auto&& view : m_allRes.bufSRV.view()) {
        if (view) {
          auto nat = view.native();
          m_device.destroyBufferView(nat.view);
        }
      }
      for (auto&& view : m_allRes.bufUAV.view()) {
        if (view) {
          auto nat = view.native();
          m_device.destroyBufferView(nat.view);
        }
      }
      for (auto&& view : m_allRes.texSRV.view()) {
        if (auto nat = view.native().view) {
          m_device.destroyImageView(nat);
        }
      }
      for (auto&& view : m_allRes.texUAV.view()) {
        if (auto nat = view.native().view) {
          m_device.destroyImageView(nat);
        }
      }
      for (auto&& view : m_allRes.texRTV.view()) {
        if (auto nat = view.native().view) {
          m_device.destroyImageView(nat);
        }
      }
      for (auto&& view : m_allRes.texDSV.view()) {
        if (auto nat = view.native().view) {
          m_device.destroyImageView(nat);
        }
      }
      // resources
      for (auto&& res : m_allRes.tex.view()) {
        if (res.canRelease()) {
          if (res.native())
            m_device.destroyImage(res.native());
          if (res.memory())
            m_device.freeMemory(res.memory());
        }
      }
      for (auto&& res : m_allRes.buf.view()) {
        if (res.native()) {
          m_device.destroyBuffer(res.native());
        }
      }
      for (auto&& res : m_allRes.rbBuf.view()) {
        if (res.native())
          m_device.destroyBuffer(res.native());
        if (res.memory())
          m_device.freeMemory(res.memory());
      }
      for (auto&& pipe : m_allRes.pipelines.view()) {
        if (pipe) {
          if (pipe.m_pipeline)
            m_device.destroyPipeline(pipe.m_pipeline);
          if (pipe.m_pipelineLayout)
            m_device.destroyPipelineLayout(pipe.m_pipelineLayout);
          m_descriptors->freeSets(m_device, makeMemView(pipe.m_staticSet));
        }
      }
      for (auto&& memory : m_allRes.heaps.view()) {
        if (memory) {
          m_device.freeMemory(memory.native());
        }
      }
      for (auto&& layout : m_allRes.shaArgsLayouts.view()) {
        if (layout) {
          m_device.destroyDescriptorSetLayout(layout.native());
        }
      }
      // clear our system resources
      // null resources
      for (auto&& format : nullFormats) {
        auto& nb = nullBuffers(format);
        auto& ni = nullImages(format);
        m_device.destroyBufferView(nb.texel);
        m_device.destroyBuffer(nb.buffer);
        m_device.destroyImageView(ni.oneDim);
        m_device.destroyImageView(ni.oneDimArray);
        m_device.destroyImageView(ni.twoDim);
        m_device.destroyImageView(ni.twoDimArray);
        m_device.destroyImageView(ni.threeDim);
        m_device.destroyImageView(ni.cube);
        m_device.destroyImageView(ni.cubeArray);
        m_device.destroyImage(ni.nullImage1d);
        m_device.destroyImage(ni.nullImageCube);
        m_device.destroyImage(ni.nullImage3d);
      }
      m_device.freeMemory(nullHeapBuffers);
      m_device.freeMemory(nullHeapImages);
      // rest
      m_fences.clear();
      m_semaphores.clear();

      m_queryPoolPool.clear();
      m_computeQueryPoolPool.clear();
      m_dmaQueryPoolPool.clear();
      m_readbackPool.clear();
      m_copyListPool.clear();
      m_computeListPool.clear();
      m_graphicsListPool.clear();
      m_renderpasses.clear();
      m_dynamicUpload.reset();
      m_constantAllocators.reset();
      m_device.destroyDescriptorSetLayout(m_defaultDescriptorLayout.native());

      m_device.destroySampler(m_samplers.m_bilinearSampler);
      m_device.destroySampler(m_samplers.m_pointSampler);
      m_device.destroySampler(m_samplers.m_bilinearSamplerWrap);
      m_device.destroySampler(m_samplers.m_pointSamplerWrap);
      m_device.destroyDescriptorPool(m_descriptors->native());

      m_device.destroy();
    }

    DeviceStatistics VulkanDevice::statsOfResourcesInUse()
    {
      DeviceStatistics stats = {};
      stats.maxConstantsUploadMemory = m_constantAllocators->max_size();
      stats.constantsUploadMemoryInUse = m_constantAllocators->size_allocated();
      stats.maxGenericUploadMemory = m_dynamicUpload->max_size();
      stats.genericUploadMemoryInUse = m_dynamicUpload->size_allocated();
      stats.descriptorsInShaderArguments = true;
      stats.descriptorsAllocated = m_descriptorSetsInUse;
      stats.maxDescriptors = m_maxDescriptorSets;
      return stats;
    }

    vk::PresentModeKHR presentModeToVk(PresentMode mode)
    {
      switch (mode)
      {
      case PresentMode::Mailbox:
        return vk::PresentModeKHR::eMailbox;
        break;
      case PresentMode::Fifo:
        return vk::PresentModeKHR::eFifo;
        break;
      case PresentMode::FifoRelaxed:
        return vk::PresentModeKHR::eFifoRelaxed;
        break;
      case PresentMode::Immediate:
      default:
        return vk::PresentModeKHR::eImmediate;
        break;
      }
    }
    PresentMode presentModeVKToHigan(vk::PresentModeKHR mode)
    {
      switch (mode)
      {
      case vk::PresentModeKHR::eMailbox:
        return PresentMode::Mailbox;
        break;
      case vk::PresentModeKHR::eFifo:
        return PresentMode::Fifo;
        break;
      case vk::PresentModeKHR::eFifoRelaxed:
        return PresentMode::FifoRelaxed;
        break;
      case vk::PresentModeKHR::eImmediate:
      default:
        return PresentMode::Immediate;
        break;
      }
    }

    bool vsyncMode(PresentMode mode)
    {
      return mode == PresentMode::Fifo || mode == PresentMode::FifoRelaxed;
    }

    template <typename Dispatch>
    vk::PresentModeKHR getValidPresentMode(vk::PhysicalDevice dev, vk::SurfaceKHR surface, Dispatch dyncamic, PresentMode mode)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto asd = dev.getSurfacePresentModesKHR(surface, dyncamic);
      VK_CHECK_RESULT(asd);
      auto presentModes = asd.value;

      vk::PresentModeKHR khrmode = presentModeToVk(mode);
      vk::PresentModeKHR similarMode = presentModeToVk(mode);
      if (mode == PresentMode::Mailbox)
      {
        similarMode = presentModeToVk(PresentMode::Immediate);
      }
      else if (mode == PresentMode::Immediate)
      {
        similarMode = presentModeToVk(PresentMode::Mailbox);
      }
      vk::PresentModeKHR backupMode;

      bool hadChosenMode = false;
      bool hadSimilarMode = false;
      for (auto&& fmt : presentModes)
      {
#ifdef        HIGANBANA_GRAPHICS_EXTRA_INFO
        if (fmt == vk::PresentModeKHR::eImmediate)
          HIGAN_SLOG("higanbana/graphics/AvailablePresentModes", "Immediate\n");
        if (fmt == vk::PresentModeKHR::eMailbox)
          HIGAN_SLOG("higanbana/graphics/AvailablePresentModes", "Mailbox\n");
        if (fmt == vk::PresentModeKHR::eFifo)
          HIGAN_SLOG("higanbana/graphics/AvailablePresentModes", "Fifo\n");
        if (fmt == vk::PresentModeKHR::eFifoRelaxed)
          HIGAN_SLOG("higanbana/graphics/AvailablePresentModes", "FifoRelaxed\n");
#endif
        if (fmt == khrmode)
          hadChosenMode = true;
        if (fmt == similarMode)
          hadSimilarMode = true;
        if (vsyncMode(presentModeVKToHigan(fmt)) && vsyncMode(mode))
        {
          backupMode = fmt;
        }
        else
        {
          backupMode = fmt;
        }
      }
      if (!hadChosenMode)
      {
        if (hadSimilarMode)
        {
          khrmode = similarMode;
        }
        else
          khrmode = backupMode; // guaranteed by spec
      }
      return khrmode;
    }

    std::shared_ptr<prototypes::SwapchainImpl> VulkanDevice::createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto format = descriptor.desc.format;
      auto buffers = descriptor.desc.bufferCount;

      auto natSurface = std::static_pointer_cast<VulkanGraphicsSurface>(surface.native());
      auto surfaceCapRes = m_physDevice.getSurfaceCapabilitiesKHR(natSurface->native());
      VK_CHECK_RESULT(surfaceCapRes);
      auto surfaceCap = surfaceCapRes.value;
#ifdef HIGANBANA_GRAPHICS_EXTRA_INFO
      HIGAN_SLOG("higanbana/graphics/Surface", "surface details\n");
      HIGAN_SLOG("higanbana/graphics/Surface", "min image Count: %d\n", surfaceCap.minImageCount);
      HIGAN_SLOG("higanbana/graphics/Surface", "current res %dx%d\n", surfaceCap.currentExtent.width, surfaceCap.currentExtent.height);
      HIGAN_SLOG("higanbana/graphics/Surface", "min res %dx%d\n", surfaceCap.minImageExtent.width, surfaceCap.minImageExtent.height);
      HIGAN_SLOG("higanbana/graphics/Surface", "max res %dx%d\n", surfaceCap.maxImageExtent.width, surfaceCap.maxImageExtent.height);
#endif

      //m_physDevice.getSurfacePresentModes2EXT

      auto formatsRes = m_physDevice.getSurfaceFormats2KHR(natSurface->native(), m_dynamicDispatch);
      VK_CHECK_RESULT(formatsRes);
      auto formats = formatsRes.value;

      auto wantedFormat = formatToVkFormat(format).storage;
      auto backupFormat = vk::Format::eB8G8R8A8Unorm;
      auto colorspace = vk::ColorSpaceKHR::eSrgbNonlinear;
      bool found = false;
      bool hadBackup = false;
      for (auto&& fmt : formats)
      {
        if (wantedFormat == fmt.surfaceFormat.format)
        {
          found = true;
          colorspace = fmt.surfaceFormat.colorSpace;
        }
        if (backupFormat == fmt.surfaceFormat.format)
        {
          colorspace = fmt.surfaceFormat.colorSpace;
          hadBackup = true;
        }
        HIGAN_SLOG("higanbana/graphics/Surface", "format: %s colorspace: %s\n", vk::to_string(fmt.surfaceFormat.format).c_str(), vk::to_string(fmt.surfaceFormat.colorSpace).c_str());
      }

      if (!found)
      {
        HIGAN_ASSERT(hadBackup, "uh oh, backup format wasn't supported either.");
        wantedFormat = backupFormat;
        format = FormatType::Unorm8BGRA;
      }
      
      auto khrmode = getValidPresentMode(m_physDevice,natSurface->native(), m_dynamicDispatch, descriptor.desc.mode);

      auto extent = surfaceCap.currentExtent;
      if (extent.height == 0)
      {
        extent.height = surfaceCap.minImageExtent.height;
      }
      if (extent.width == 0)
      {
        extent.width = surfaceCap.minImageExtent.width;
      }

      if (m_physDevice.getSurfaceSupportKHR(m_mainQueueIndex, natSurface->native()).result != vk::Result::eSuccess)
      {
        HIGAN_ASSERT(false, "Was not supported.");
      }

      int minImageCount = std::max(static_cast<int>(surfaceCap.minImageCount), buffers);
      HIGAN_SLOG("Vulkan", "creating swapchain to %ux%u, buffers %d\n", extent.width, extent.height, minImageCount);

      vk::SwapchainCreateInfoKHR info = vk::SwapchainCreateInfoKHR()
        .setSurface(natSurface->native())
        .setMinImageCount(minImageCount)
        .setImageFormat(wantedFormat)
        .setImageColorSpace(colorspace)
        .setImageExtent(surfaceCap.currentExtent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)  // linear to here
        .setImageSharingMode(vk::SharingMode::eExclusive)
        //	.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eInherit)
        //	.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eInherit)
        .setPresentMode(khrmode)
        .setClipped(false);
      auto scRes = m_device.createSwapchainKHR(info);
      VK_CHECK_RESULT(scRes);
      auto swapchain = scRes.value;
      auto sc = std::shared_ptr<VulkanSwapchain>(new VulkanSwapchain(swapchain, *natSurface, m_semaphores.allocate()),
        [dev = m_device](VulkanSwapchain* ptr)
      {
        dev.destroySwapchainKHR(ptr->native());
        delete ptr;
      });
      sc->setBufferMetadata(surfaceCap.currentExtent.width, surfaceCap.currentExtent.height, minImageCount, format, presentModeVKToHigan(khrmode));
      return sc;
    }

    void VulkanDevice::adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> swapchain, SwapchainDescriptor descriptor)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto format = descriptor.desc.format;
      auto mode = descriptor.desc.mode;
      auto bufferCount = descriptor.desc.bufferCount;

      auto natSwapchain = std::static_pointer_cast<VulkanSwapchain>(swapchain);
      auto& natSurface = natSwapchain->surface();
      auto oldSwapchain = natSwapchain->native();

      format = (format == FormatType::Unknown) ? natSwapchain->getDesc().format : format;
      bufferCount = (bufferCount == -1) ? natSwapchain->getDesc().buffers : bufferCount;
      mode = (mode == PresentMode::Unknown) ? natSwapchain->getDesc().mode : mode;

      vector<vk::SurfaceFormatKHR> formats;
      {
        HIGAN_CPU_BRACKET("vkGetSurfaceFormatsKHR");
        auto formatsRes = m_physDevice.getSurfaceFormatsKHR(natSurface.native());
        VK_CHECK_RESULT(formatsRes);
        formats = formatsRes.value;
      }

      auto wantedFormat = formatToVkFormat(format).storage;
      auto backupFormat = vk::Format::eB8G8R8A8Unorm;
      bool found = false;
      bool hadBackup = false;
      for (auto&& fmt : formats)
      {
        if (wantedFormat == fmt.format)
        {
          found = true;
        }
        if (backupFormat == fmt.format)
        {
          hadBackup = true;
        }
        //HIGAN_SLOG("higanbana/graphics/Surface", "format: %s\n", vk::to_string(fmt.format).c_str());
      }

      if (!found)
      {
        HIGAN_ASSERT(hadBackup, "uh oh, backup format wasn't supported either.");
        wantedFormat = backupFormat;
        format = FormatType::Unorm8BGRA;
      }

      vk::SurfaceCapabilitiesKHR surfaceCap;
      {
        HIGAN_CPU_BRACKET("vkGetSurfaceCapabilitiesKHR");
        auto surfaceCapRes = m_physDevice.getSurfaceCapabilitiesKHR(natSurface.native());
        VK_CHECK_RESULT(surfaceCapRes);
        surfaceCap = surfaceCapRes.value;
      }

      auto& extent = surfaceCap.currentExtent;
      if (extent.height < 8)
      {
        extent.height = 8;
      }
      if (extent.width < 8)
      {
        extent.width = 8;
      }

      int minImageCount = std::max(static_cast<int>(surfaceCap.minImageCount), bufferCount);

      {
        HIGAN_CPU_BRACKET("vkGetSurfaceSupportKHR");
        if (m_physDevice.getSurfaceSupportKHR(m_mainQueueIndex, natSurface.native()).result != vk::Result::eSuccess)
        {
          HIGAN_ASSERT(false, "Was not supported.");
        }
      }
      vk::PresentModeKHR khrmode;
      {
        HIGAN_CPU_BRACKET("vkGetSurfaceSupportKHR");
        khrmode = getValidPresentMode(m_physDevice, natSurface.native(), m_dynamicDispatch, mode);
      }

      vk::SwapchainCreateInfoKHR info = vk::SwapchainCreateInfoKHR()
        .setSurface(natSurface.native())
        .setMinImageCount(minImageCount)
        .setImageFormat(formatToVkFormat(format).storage)
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent(surfaceCap.currentExtent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)  // linear to here
        .setImageSharingMode(vk::SharingMode::eExclusive)
        //	.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eInherit)
        //	.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eInherit)
        .setPresentMode(khrmode)
        .setClipped(false)
        .setOldSwapchain(oldSwapchain);

      {
        HIGAN_CPU_BRACKET("vkCreateSwapchainKHR");
        auto scRes = m_device.createSwapchainKHR(info);
        VK_CHECK_RESULT(scRes);
        natSwapchain->setSwapchain(scRes.value);
      }

      {
        HIGAN_CPU_BRACKET("vkDeviceDestroySwapchainKHR");
        m_device.destroySwapchainKHR(oldSwapchain);
      }
      //HIGAN_SLOG("Vulkan", "adjusting swapchain to %ux%u\n", surfaceCap.currentExtent.width, surfaceCap.currentExtent.height);
      natSwapchain->setBufferMetadata(surfaceCap.currentExtent.width, surfaceCap.currentExtent.height, minImageCount, format, presentModeVKToHigan(khrmode));
      natSwapchain->setOutOfDate(false);
    }

    int VulkanDevice::fetchSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc, vector<ResourceHandle>& handles)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto native = std::static_pointer_cast<VulkanSwapchain>(sc);
      auto imagesRes = m_device.getSwapchainImagesKHR(native->native());
      VK_CHECK_RESULT(imagesRes);
      auto images = imagesRes.value;
      vector<TextureStateFlags> state;
      state.emplace_back(TextureStateFlags(vk::AccessFlagBits(0), vk::ImageLayout::eUndefined, m_mainQueueIndex));

      for (int i = 0; i < static_cast<int>(images.size()); ++i)
      {
        //textures[i] = std::make_shared<VulkanTexture>(images[i], std::make_shared<VulkanTextureState>(VulkanTextureState{ state }), false);
        m_allRes.tex[handles[i]] = VulkanTexture(images[i], sc->desc(), false);
      }
      
      return static_cast<int>(images.size());
    }

    int VulkanDevice::tryAcquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      HIGAN_CPU_BRACKET("Vulkan try AcquireImage");
      auto native = std::static_pointer_cast<VulkanSwapchain>(swapchain);

      if (native->outOfDate())
        return -1;

      std::shared_ptr<VulkanSemaphore> freeSemaphore = m_semaphores.allocate();
      auto res = m_device.acquireNextImageKHR(native->native(), 1000, freeSemaphore->native(), nullptr);

      if (res.result != vk::Result::eSuccess)
      {
        native->setOutOfDate(true);
        if (res.result != vk::Result::eSuboptimalKHR)
        {
          HIGAN_ILOG("Vulkan/AcquireNextImage", "error: %s\n", to_string(res.result).c_str());
          return -1;
        }
      }
      native->setCurrentPresentableImageIndex(res.value);
      native->setAcquireSemaphore(freeSemaphore);

      return res.value;
    }

    // TODO: add fence here, so that we can detect that "we cannot render yet, do something else". Bonus thing honestly.
    int VulkanDevice::acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> sc)
    {
      HIGAN_CPU_BRACKET("Vulkan AcquireImage");
      auto native = std::static_pointer_cast<VulkanSwapchain>(sc);
      if (native->outOfDate())
        return -1;

      std::shared_ptr<VulkanSemaphore> freeSemaphore = m_semaphores.allocate();
      auto res = m_device.acquireNextImageKHR(native->native(), (std::numeric_limits<uint64_t>::max)(), freeSemaphore->native(), nullptr);

      // note on intel, this never fails or something =D so we don't know if we need to resize or not
      if (res.result != vk::Result::eSuccess)
      {
        HIGAN_LOGi("Vulkan/AcquireNextImage got failure!\n");
        native->setOutOfDate(true);
        if (res.result != vk::Result::eSuboptimalKHR)
        {
          HIGAN_ILOG("Vulkan/AcquireNextImage", "error: %s\n", to_string(res.result).c_str());
          return -1;
        }
      }
      native->setCurrentPresentableImageIndex(res.value);
      native->setAcquireSemaphore(freeSemaphore);

      return res.value;
    }

    vk::RenderPass VulkanDevice::createRenderpass(const vk::RenderPassCreateInfo& info)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto attachmentHash = HashMemory(info.pAttachments, info.attachmentCount * sizeof(vk::AttachmentDescription));
      auto dependencyHash = HashMemory(info.pDependencies, info.dependencyCount * sizeof(vk::SubpassDependency));
      auto subpassHash = HashMemory(info.pSubpasses, info.subpassCount * sizeof(vk::SubpassDescription));
      auto totalHash = hash_combine(hash_combine(attachmentHash, dependencyHash), subpassHash);

      auto found = m_renderpasses.find(totalHash);
      if (found != m_renderpasses.end())
      {
        GFX_LOG("Reusing old renderpass.\n");
        return *found->second;
      }

      auto renderpassRes = m_device.createRenderPass(info);
      VK_CHECK_RESULT(renderpassRes);
      auto renderpass = renderpassRes.value;

      auto newRP = std::shared_ptr<vk::RenderPass>(new vk::RenderPass(renderpass), [dev = m_device](vk::RenderPass* ptr)
      {
        dev.destroyRenderPass(*ptr);
        delete ptr;
      });

      m_renderpasses[totalHash] = newRP;

      GFX_LOG("Created new renderpass object.\n");
      return *newRP;
    }

    vk::ShaderStageFlagBits shaderTypeToVulkan(ShaderType type)
    {
      switch (type)
      {
        case ShaderType::Vertex: return vk::ShaderStageFlagBits::eVertex;
        case ShaderType::Geometry: return vk::ShaderStageFlagBits::eGeometry;
        case ShaderType::Hull: return vk::ShaderStageFlagBits::eTessellationControl;
        case ShaderType::Domain: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case ShaderType::Pixel: return vk::ShaderStageFlagBits::eFragment;
        case ShaderType::Amplification: return vk::ShaderStageFlagBits::eTaskNV;
        case ShaderType::Mesh: return vk::ShaderStageFlagBits::eMeshNV;
        case ShaderType::Compute: return vk::ShaderStageFlagBits::eCompute;
        case ShaderType::Raytracing:
        {
          HIGAN_ASSERT(false, "not sure what to do here...");
          return vk::ShaderStageFlagBits();
        }
        case ShaderType::Unknown: HIGAN_ASSERT(false, "Unknown shaders halp");
        default:
          return vk::ShaderStageFlagBits();
      }
    }

    void VulkanDevice::getGfxPipelineInformation(vk::Pipeline pipe, higanbana::GraphicsPipelineDescriptor::Desc& d) {
      if (m_dynamicDispatch.vkGetPipelineExecutablePropertiesKHR)
      {
        auto execProp = m_device.getPipelineExecutablePropertiesKHR(vk::PipelineInfoKHR().setPipeline(pipe), m_dynamicDispatch);
        VK_CHECK_RESULT(execProp);
        vector<vk::PipelineExecutableInfoKHR> execInfos;
        int execIndex = 0;
        for (auto&& prop : execProp.value)
        {
          //HIGAN_LOGi("Pipeline Prop \"%s\": \"%s\" %s\n", prop.name, prop.description, vk::to_string(prop.stages).c_str());
          execInfos.emplace_back(vk::PipelineExecutableInfoKHR()
            .setPipeline(pipe)
            .setExecutableIndex(execIndex++));
        }

        execIndex = 0;
        for (auto&& einfo : execInfos)
        {
          auto exeresult = m_device.getPipelineExecutableStatisticsKHR(einfo, m_dynamicDispatch);
          VK_CHECK_RESULT(exeresult);
          auto curIndex = execIndex++;
          std::string something = (curIndex < d.shaders.size()) ? d.shaders[curIndex].second : "Unknown";
          HIGAN_LOGi("Shader \"%s\" stats: %s\n", something.c_str(), execProp.value[curIndex].description);
          for (auto&& exeStats : exeresult.value)
          {
            HIGAN_LOGi("\t \"%s\": \"%s\" value: ", exeStats.name, exeStats.description);
            switch (exeStats.format)
            {
              case vk::PipelineExecutableStatisticFormatKHR::eBool32:
                HIGAN_LOGi("%s\n", exeStats.value.b32 ? "true" : "false");
                break;
              case vk::PipelineExecutableStatisticFormatKHR::eInt64:
                HIGAN_LOGi("%zd\n", exeStats.value.i64);
                break;
              case vk::PipelineExecutableStatisticFormatKHR::eUint64:
                HIGAN_LOGi("%zu\n", exeStats.value.u64);
                break;
              case vk::PipelineExecutableStatisticFormatKHR::eFloat64:
              default:
                HIGAN_LOGi("%.3f\n", exeStats.value.f64);
            }
          }
        }
      }
    }


    void VulkanDevice::getComputePipelineInformation(vk::Pipeline pipe, higanbana::ComputePipelineDescriptor& d) {
      if (m_dynamicDispatch.vkGetPipelineExecutablePropertiesKHR)
      {
        auto execProp = m_device.getPipelineExecutablePropertiesKHR(vk::PipelineInfoKHR().setPipeline(pipe), m_dynamicDispatch);
        VK_CHECK_RESULT(execProp);
        vector<vk::PipelineExecutableInfoKHR> execInfos;
        int execIndex = 0;
        for (auto&& prop : execProp.value)
        {
          //HIGAN_LOGi("Pipeline Prop \"%s\": \"%s\" %s\n", prop.name, prop.description, vk::to_string(prop.stages).c_str());
          execInfos.emplace_back(vk::PipelineExecutableInfoKHR()
            .setPipeline(pipe)
            .setExecutableIndex(execIndex++));
        }

        execIndex = 0;
        for (auto&& einfo : execInfos)
        {
          auto exeresult = m_device.getPipelineExecutableStatisticsKHR(einfo, m_dynamicDispatch);
          VK_CHECK_RESULT(exeresult);
          auto curIndex = execIndex++;
          HIGAN_LOGi("Shader \"%s\" stats: %s\n", d.shader(), execProp.value[curIndex].description);
          for (auto&& exeStats : exeresult.value)
          {
            HIGAN_LOGi("\t \"%s\": \"%s\" value: ", exeStats.name, exeStats.description);
            switch (exeStats.format)
            {
              case vk::PipelineExecutableStatisticFormatKHR::eBool32:
                HIGAN_LOGi("%s\n", exeStats.value.b32 ? "true" : "false");
                break;
              case vk::PipelineExecutableStatisticFormatKHR::eInt64:
                HIGAN_LOGi("%zd\n", exeStats.value.i64);
                break;
              case vk::PipelineExecutableStatisticFormatKHR::eUint64:
                HIGAN_LOGi("%zu\n", exeStats.value.u64);
                break;
              case vk::PipelineExecutableStatisticFormatKHR::eFloat64:
              default:
                HIGAN_LOGi("%.3f\n", exeStats.value.f64);
            }
          }
        }
      }

    }

    std::optional<vk::Pipeline> VulkanDevice::updatePipeline(ResourceHandle pipeline, gfxpacket::RenderPassBegin& rpbegin)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      if (m_allRes.pipelines[pipeline].m_hasPipeline->load() && !m_allRes.pipelines[pipeline].needsUpdating())
        return {};

      std::lock_guard<std::mutex> pipelineUpdateLock(m_deviceLock);
      auto& vp = m_allRes.pipelines[pipeline];
      if (vp.m_hasPipeline->load() && !vp.needsUpdating())
        return {};

      Timer pipelineRecreationTime;
      auto& desc = vp.m_gfxDesc;

      struct ReadyShader
      {
        vk::ShaderStageFlagBits stage;
        vk::ShaderModule module;
      };
      vector<ReadyShader> shaders;

      auto& d = desc.desc;

      for (auto&& [shaderType, sourcePath] : d.shaders)
      {
        bool forceCompile = false;
        for (auto& shader : vp.m_watchedShaders) {
          if (shader.second == shaderType) {
            forceCompile = shader.first.updated();
            break;
          }
        }
        auto sci = ShaderCreateInfo(sourcePath, shaderType, d.layout);
        if (forceCompile) sci = sci.compile();
        auto shader = m_shaders.shader(sci);
        vk::ShaderModuleCreateInfo si = vk::ShaderModuleCreateInfo()
          .setCodeSize(shader.size())
          .setPCode(reinterpret_cast<uint32_t*>(shader.data()));
        auto module = m_device.createShaderModule(si);
        VK_CHECK_RESULT(module);

        ReadyShader ss;
        ss.stage = shaderTypeToVulkan(shaderType);
        ss.module = module.value;
        shaders.push_back(ss);
        auto found = std::find_if(vp.m_watchedShaders.begin(), vp.m_watchedShaders.end(), [stype = shaderType](std::pair<WatchFile, ShaderType>& shader) {
            if (shader.second == stype)
            {
              shader.first.react();
              return true;
            }
            return false;
          });
        if (found == vp.m_watchedShaders.end())
        {
          vp.m_watchedShaders.push_back(std::make_pair(m_shaders.watch(sourcePath, shaderType), shaderType));
        }
      }

      vector<vk::PipelineShaderStageCreateInfo> shaderInfos;

      for (auto&& it : shaders)
      {
        shaderInfos.push_back(vk::PipelineShaderStageCreateInfo()
        .setPName("main")
        .setStage(it.stage)
        .setModule(it.module));
      }

      vector<vk::DynamicState> dynamics = {
        vk::DynamicState::eViewport, 
        vk::DynamicState::eScissor,
        vk::DynamicState::eDepthBounds};
      auto dynamicsInfo = vk::PipelineDynamicStateCreateInfo()
        .setDynamicStateCount(dynamics.size()).setPDynamicStates(dynamics.data());

      auto vertexInput = vk::PipelineVertexInputStateCreateInfo();

      auto blendAttachments = getBlendAttachments(desc.desc.blendDesc, desc.desc.numRenderTargets);
      auto blendInfo = getBlendStateDesc(desc.desc.blendDesc, blendAttachments);
      auto rasterInfo = getRasterStateDesc(desc.desc.rasterDesc);
      auto depthInfo = getDepthStencilDesc(desc.desc.dsdesc);
      auto inputInfo = getInputAssemblyDesc(desc);
      auto msaainfo = getMultisampleDesc(desc);
      auto viewport = vk::Viewport();
      auto scissor = vk::Rect2D();
      auto viewportInfo = vk::PipelineViewportStateCreateInfo()
      .setPScissors(&scissor)
      .setScissorCount(1)
      .setPViewports(&viewport)
      .setViewportCount(1);

      auto rp = m_allRes.renderpasses[rpbegin.renderpass].native();
      vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo()
        .setFlags(vk::PipelineCreateFlagBits::eCaptureStatisticsKHR)
        .setLayout(vp.m_pipelineLayout)
        .setStageCount(shaderInfos.size())
        .setPStages(shaderInfos.data())
        .setPVertexInputState(&vertexInput)
        .setPViewportState(&viewportInfo)
        .setPColorBlendState(&blendInfo)
        .setPDepthStencilState(&depthInfo)
        .setPInputAssemblyState(&inputInfo)
        .setPMultisampleState(&msaainfo)
        .setPRasterizationState(&rasterInfo)
        .setPDynamicState(&dynamicsInfo)
        .setRenderPass(rp)
        .setSubpass(0); // only 1 ever available

      if (m_dynamicDispatch.vkGetPipelineExecutablePropertiesKHR)
      {
        pipelineInfo = pipelineInfo.setFlags(vk::PipelineCreateFlagBits::eCaptureStatisticsKHR);
      }


      auto compiled = m_device.createGraphicsPipeline(nullptr, pipelineInfo);
      VK_CHECK_RESULT(compiled);

      setDebugUtilsObjectNameEXT(compiled.value, desc.desc.shaders.begin()->second.c_str());

      for (auto&& it : shaders)
      {
        m_device.destroyShaderModule(it.module);
      }
      std::optional<vk::Pipeline> ret;
      if (vp.m_hasPipeline)
      {
        ret = vp.m_pipeline;
      }
      vp.m_pipeline = compiled.value;
      vp.m_hasPipeline->store(true);

      getGfxPipelineInformation(vp.m_pipeline, d);
      
      HIGAN_ILOG("Vulkan", "Graphics Pipeline \"%s\" created in %.2fms", d.shaders.back().second.c_str(), float(pipelineRecreationTime.timeFromLastReset()) / 1000000.f);

      return ret;
    }

    std::optional<vk::Pipeline> VulkanDevice::updatePipeline(ResourceHandle pipeline)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      if (m_allRes.pipelines[pipeline].m_hasPipeline->load() && !m_allRes.pipelines[pipeline].cs.updated())
        return {};

      std::lock_guard<std::mutex> pipelineUpdateLock(m_deviceLock);
      auto& pipe = m_allRes.pipelines[pipeline];
      if (pipe.m_hasPipeline->load() && !pipe.cs.updated())
        return {};
      Timer pipelineRecreationTime;

      auto sci = ShaderCreateInfo(pipe.m_computeDesc.shader(), ShaderType::Compute, pipe.m_computeDesc.layout)
        .setComputeGroups(pipe.m_computeDesc.shaderGroups);
      if (pipe.cs.updated())
        sci = sci.compile();
      auto shader = m_shaders.shader(sci);

      auto shaderInfo = vk::ShaderModuleCreateInfo().setCodeSize(shader.size()).setPCode(reinterpret_cast<uint32_t*>(shader.data()));

      auto smodule = m_device.createShaderModule(shaderInfo);
      VK_CHECK_RESULT(smodule);
      if (pipe.cs.empty())
      {
        pipe.cs = m_shaders.watch(pipe.m_computeDesc.shader(), ShaderType::Compute);
      }
      pipe.cs.react();

      auto pipelineDesc = vk::ComputePipelineCreateInfo()
        .setFlags(vk::PipelineCreateFlagBits::eCaptureStatisticsKHR)
        .setStage(vk::PipelineShaderStageCreateInfo()
          .setModule(smodule.value)
          .setPName("main")
          .setStage(vk::ShaderStageFlagBits::eCompute))
        .setLayout(pipe.m_pipelineLayout);
      auto result = m_device.createComputePipeline(nullptr, pipelineDesc);
      m_device.destroyShaderModule(smodule.value);

      setDebugUtilsObjectNameEXT(result.value, pipe.m_computeDesc.shader());

      std::optional<vk::Pipeline> oldPipe;
      if (result.result == vk::Result::eSuccess)
      {
        if (pipe.m_hasPipeline)
        {
          oldPipe = pipe.m_pipeline;
        }
        pipe.m_pipeline = result.value;
        pipe.m_hasPipeline->store(true);
      }
      getComputePipelineInformation(pipe.m_pipeline, pipe.m_computeDesc);
      HIGAN_ILOG("Vulkan", "Compute Pipeline \"%s\" created in %.2fms", pipe.m_computeDesc.shader(), float(pipelineRecreationTime.timeFromLastReset()) / 1000000.f);
      return oldPipe;
    }

    VulkanQueryPool VulkanDevice::createGraphicsQueryPool(unsigned counters)
    {
      return VulkanQueryPool(m_device, m_physDevice, m_limits, m_mainQueueIndex, vk::QueryType::eTimestamp, counters);
    }

    VulkanQueryPool VulkanDevice::createComputeQueryPool(unsigned counters)
    {
      return VulkanQueryPool(m_device, m_physDevice, m_limits, m_computeQueueIndex, vk::QueryType::eTimestamp, counters);
    }

    VulkanQueryPool VulkanDevice::createDMAQueryPool(unsigned counters)
    {
      return VulkanQueryPool(m_device, m_physDevice, m_limits, m_copyQueueIndex, vk::QueryType::eTimestamp, counters);
    }

    VulkanReadbackHeap VulkanDevice::createReadback(unsigned pages, unsigned pageSize)
    {
      return VulkanReadbackHeap(m_device, m_physDevice,memoryAddressingFlags(), pages, pageSize);
    }

    void VulkanDevice::createPipeline(ResourceHandle handle, GraphicsPipelineDescriptor desc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      vector<vk::DescriptorSetLayout> layouts;
      for (auto&& layout : desc.desc.layout.m_sets)
      {
        layouts.push_back(m_allRes.shaArgsLayouts[layout.handle()].native());
      }
      layouts.push_back(m_defaultDescriptorLayout.native());

      auto pipelineLayout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(layouts.size())
        .setPSetLayouts(layouts.data()));
      VK_CHECK_RESULT(pipelineLayout);

      setDebugUtilsObjectNameEXT(pipelineLayout.value, desc.desc.shaders.front().second.c_str());

      auto defLayout = defaultDescLayout();
      auto aainfo = vk::DescriptorSetAllocateInfo()
        .setDescriptorSetCount(1)
        .setPSetLayouts(&defLayout);

      auto set = m_descriptors->allocate(m_device, aainfo);

      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(m_constantAllocators->buffer())
        .setOffset(0)
        .setRange(1024);

      int index = 0;
      vk::WriteDescriptorSet wds = vk::WriteDescriptorSet()
        .setDstSet(set[0])
        .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
        .setDescriptorCount(1)
        .setPBufferInfo(&info)
        .setDstBinding(0);

      vk::DescriptorBufferInfo shDebug = vk::DescriptorBufferInfo()
        .setBuffer(m_shaderDebugBuffer.native())
        .setOffset(0)
        .setRange(HIGANBANA_SHADER_DEBUG_WIDTH);

      vk::WriteDescriptorSet shaderDebug = vk::WriteDescriptorSet()
        .setDstSet(set[0])
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setDescriptorCount(1)
        .setPBufferInfo(&shDebug)
        .setDstBinding(1);

      m_device.updateDescriptorSets({wds, shaderDebug}, {});

      m_allRes.pipelines[handle] = VulkanPipeline(pipelineLayout.value, desc, set[0]);
    }

    void VulkanDevice::createPipeline(ResourceHandle handle, ComputePipelineDescriptor desc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      vector<vk::DescriptorSetLayout> layouts;
      for (auto&& layout : desc.layout.m_sets)
      {
        layouts.push_back(m_allRes.shaArgsLayouts[layout.handle()].native());
      }
      layouts.push_back(m_defaultDescriptorLayout.native());

      auto pipelineLayout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(layouts.size())
        .setPSetLayouts(layouts.data()));
      VK_CHECK_RESULT(pipelineLayout);

      setDebugUtilsObjectNameEXT(pipelineLayout.value, desc.shader());

      auto defLayout = defaultDescLayout();
      auto aainfo = vk::DescriptorSetAllocateInfo()
        .setDescriptorSetCount(1)
        .setPSetLayouts(&defLayout);

      auto set = m_descriptors->allocate(m_device, aainfo);
      m_descriptorSetsInUse++;

      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(m_constantAllocators->buffer())
        .setOffset(0)
        .setRange(1024);

      int index = 0;
      vk::WriteDescriptorSet wds = vk::WriteDescriptorSet()
        .setDstSet(set[0])
        .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
        .setDescriptorCount(1)
        .setPBufferInfo(&info)
        .setDstBinding(0);
      vk::DescriptorBufferInfo shDebug = vk::DescriptorBufferInfo()
        .setBuffer(m_shaderDebugBuffer.native())
        .setOffset(0)
        .setRange(HIGANBANA_SHADER_DEBUG_WIDTH);

      vk::WriteDescriptorSet shaderDebug = vk::WriteDescriptorSet()
        .setDstSet(set[0])
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setDescriptorCount(1)
        .setPBufferInfo(&shDebug)
        .setDstBinding(1);

      m_device.updateDescriptorSets({wds, shaderDebug}, {});

      m_allRes.pipelines[handle] = VulkanPipeline( pipelineLayout.value, desc, set[0]);
    }
    void VulkanDevice::createPipeline(ResourceHandle handle, RaytracingPipelineDescriptor desc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();

      m_allRes.pipelines[handle] = VulkanPipeline();
    }

    vector<vk::DescriptorSetLayoutBinding> VulkanDevice::defaultSetLayoutBindings(vk::ShaderStageFlags flags)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      vector<vk::DescriptorSetLayoutBinding> bindings;

      int slot = 0;
      bindings.push_back(vk::DescriptorSetLayoutBinding()
        .setBinding(slot++).setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
        .setStageFlags(flags));
      bindings.push_back(vk::DescriptorSetLayoutBinding()
        .setBinding(slot++).setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setStageFlags(flags));
      bindings.push_back(vk::DescriptorSetLayoutBinding()
        .setBinding(slot++).setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setPImmutableSamplers(&m_samplers.m_bilinearSampler)
        .setStageFlags(flags));
      bindings.push_back(vk::DescriptorSetLayoutBinding()
        .setBinding(slot++).setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setPImmutableSamplers(&m_samplers.m_pointSampler)
        .setStageFlags(flags));
      bindings.push_back(vk::DescriptorSetLayoutBinding()
        .setBinding(slot++).setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setPImmutableSamplers(&m_samplers.m_bilinearSamplerWrap)
        .setStageFlags(flags));
      bindings.push_back(vk::DescriptorSetLayoutBinding()
        .setBinding(slot++).setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setPImmutableSamplers(&m_samplers.m_pointSamplerWrap)
        .setStageFlags(flags));

      return bindings;
    }

    vector<vk::DescriptorSetLayoutBinding> VulkanDevice::gatherSetLayoutBindings(ShaderArgumentsLayoutDescriptor desc, vk::ShaderStageFlags flags)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto layout = desc;
      vector<vk::DescriptorSetLayoutBinding> bindings;

      int slot = 0;

      auto resources = layout.getResources();

      auto getDescriptor = [](ShaderResourceType type, bool ro)
      {
        if (type == ShaderResourceType::Buffer)
        {
          if (ro)
          {
            return vk::DescriptorType::eUniformTexelBuffer;
          }
          else
          {
            return vk::DescriptorType::eStorageTexelBuffer;
          }
        }
        else if (type == ShaderResourceType::ByteAddressBuffer || type == ShaderResourceType::StructuredBuffer)
        {
          return vk::DescriptorType::eStorageBuffer;
        }
        else
        {
          if (ro)
          {
            return vk::DescriptorType::eSampledImage;
          }
          else
          {
            return vk::DescriptorType::eStorageImage;
          }
        }
      };

      vk::DescriptorType currentType;
      int descCount = 0;
      for (auto&& it : resources)
      {
        bindings.push_back(vk::DescriptorSetLayoutBinding()
          .setBinding(slot++)
          .setDescriptorCount(1) // TODO: if the descriptor is array, then touch this
          .setDescriptorType(getDescriptor(it.type, it.readonly))
          .setStageFlags(flags));
      }
      if (!layout.bindless.name.empty()) {
        bindings.push_back(vk::DescriptorSetLayoutBinding()
          .setBinding(slot)
          .setDescriptorCount(layout.bindless.bindlessCountWorstCase)
          .setDescriptorType(getDescriptor(layout.bindless.type, layout.bindless.readonly))
          .setStageFlags(flags));
      }

      return bindings;
    }

    void VulkanDevice::releaseHandle(ResourceHandle handle)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      switch(handle.type)
      {
        case ResourceType::Buffer:
        {
          m_device.destroyBuffer(m_allRes.buf[handle].native());
          m_allRes.buf[handle] = VulkanBuffer();
          break;
        }
        case ResourceType::Texture:
        {
          auto& tex = m_allRes.tex[handle];
          if (tex.canRelease())
            m_device.destroyImage(tex.native());

          if (tex.hasDedicatedMemory())
            m_device.freeMemory(tex.memory());
          m_allRes.tex[handle] = VulkanTexture();
          break;
        }
        case ResourceType::Pipeline:
        {
          m_device.destroyPipeline(m_allRes.pipelines[handle].m_pipeline);
          m_device.destroyPipelineLayout(m_allRes.pipelines[handle].m_pipelineLayout);
          m_descriptors->freeSets(m_device, makeMemView(m_allRes.pipelines[handle].m_staticSet));
          m_allRes.pipelines[handle] = VulkanPipeline();
          break;
        }
        case ResourceType::DynamicBuffer:
        {
          auto& dyn = m_allRes.dynBuf[handle];
          m_dynamicUpload->release(dyn.native().block);
          if (dyn.native().texelView)
          {
            m_device.destroyBufferView(dyn.native().texelView);
          }
          m_dynamicUpload->release(dyn.native().block);
          dyn = VulkanDynamicBufferView();
          break;
        }
        case ResourceType::MemoryHeap:
        {
          m_device.freeMemory(m_allRes.heaps[handle].native());
          m_allRes.heaps[handle] = VulkanHeap();
          break;
        }
        case ResourceType::Renderpass:
        {
          //m_device.destroyRenderPass(m_allRes.renderpasses[handle].native());
          m_allRes.renderpasses[handle] = VulkanRenderpass();
          break;
        }
        case ResourceType::ReadbackBuffer:
        {
          m_device.destroyBuffer(m_allRes.rbBuf[handle].native());
          m_device.freeMemory(m_allRes.rbBuf[handle].memory());
          m_allRes.rbBuf[handle] = VulkanReadback();
          break;
        }
        case ResourceType::ShaderArgumentsLayout:
        {
          m_device.destroyDescriptorSetLayout(m_allRes.shaArgsLayouts[handle].native());
          m_allRes.shaArgsLayouts[handle] = VulkanShaderArgumentsLayout();
          break;
        }
        case ResourceType::ShaderArguments:
        {
          auto& dyn = m_allRes.shaArgs[handle];
          auto set = dyn.native();
          m_descriptorSetsInUse--;
          m_descriptors->freeSets(m_device, makeMemView(set));
          dyn = VulkanShaderArguments();
          break;
        }
        default:
        {
          HIGAN_ASSERT(false, "unhandled type released");
          break;
        }
      }
    }

    void VulkanDevice::releaseViewHandle(ViewResourceHandle handle)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      switch(handle.type)
      {
        case ViewResourceType::BufferIBV:
        {
          m_device.destroyBufferView(m_allRes.bufIBV[handle].native().view);
          m_allRes.bufIBV[handle] = VulkanBufferView();
          break;
        }
        case ViewResourceType::BufferSRV:
        {
          auto view = m_allRes.bufSRV[handle].native();
          if (view.view)
          {
            m_device.destroyBufferView(view.view);
          }
          m_allRes.bufSRV[handle] = VulkanBufferView();
          break;
        }
        case ViewResourceType::BufferUAV:
        {
          auto view = m_allRes.bufUAV[handle].native();
          if (view.view)
          {
            m_device.destroyBufferView(view.view);
          }
          m_allRes.bufUAV[handle] = VulkanBufferView();
          break;
        }
        case ViewResourceType::RaytracingAccelerationStructure:
        {
          HIGAN_ASSERT(false, "unimplemented");
          break;
        }
        case ViewResourceType::TextureSRV:
        {
          m_device.destroyImageView(m_allRes.texSRV[handle].native().view);
          m_allRes.texSRV[handle] = VulkanTextureView();
          break;
        }
        case ViewResourceType::TextureUAV:
        {
          m_device.destroyImageView(m_allRes.texUAV[handle].native().view);
          m_allRes.texUAV[handle] = VulkanTextureView();
          break;
        }
        case ViewResourceType::TextureRTV:
        {
          m_device.destroyImageView(m_allRes.texRTV[handle].native().view);
          m_allRes.texRTV[handle] = VulkanTextureView();
          break;
        }
        case ViewResourceType::TextureDSV:
        {
          m_device.destroyImageView(m_allRes.texDSV[handle].native().view);
          m_allRes.texDSV[handle] = VulkanTextureView();
          break;
        }
        case ViewResourceType::DynamicBufferSRV:
        {
          auto& dyn = m_allRes.dynBuf[handle];
          if (dyn.native().texelView)
          {
            m_device.destroyBufferView(dyn.native().texelView);
          }
          m_dynamicUpload->release(dyn.native().block);
          dyn = VulkanDynamicBufferView();
          break;
        }
        default:
        {
          HIGAN_ASSERT(false, "unhandled type released");
          break;
        }
      }
    }

    void VulkanDevice::waitGpuIdle()
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      VK_CHECK_RESULT_RAW(m_device.waitIdle());
    }

    MemoryRequirements VulkanDevice::getReqs(ResourceDescriptor desc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      MemoryRequirements reqs{};
      vk::MemoryRequirements requirements;
      if (desc.desc.dimension == FormatDimension::Buffer)
      {
        auto buffer = m_device.createBuffer(fillBufferInfo(desc, m_memoryAddressingEnabled, m_limits.minStorageBufferOffsetAlignment));
        VK_CHECK_RESULT(buffer);
        requirements = m_device.getBufferMemoryRequirements(buffer.value);
        m_device.destroyBuffer(buffer.value);
      }
      else
      {
        auto imageInfo = fillImageInfo(desc);
        auto image = m_device.createImage(imageInfo);
        VK_CHECK_RESULT(image);
        requirements = m_device.getImageMemoryRequirements(image.value);
        m_device.destroyImage(image.value);
      }
      auto memProp = m_physDevice.getMemoryProperties();
      auto searchProperties = getMemoryProperties(desc.desc.usage);
      auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
      HIGAN_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?"); // searchProperties.def

      int32_t customBit = 0;
      if (desc.desc.allowCrossAdapter)
      {
        customBit = 1;
      }

      auto packed = packInt64(index, customBit);
      //int32_t v1, v2;
      //unpackInt64(packed, v1, v2);
      //HIGAN_ILOG("Vulkan", "Did (unnecessary) packing tricks, packed %d -> %zd -> %d", index, packed, v1);
      reqs.heapType = packed;
      reqs.alignment = requirements.alignment;
      reqs.bytes = requirements.size;

      return reqs;
    }

    void VulkanDevice::createRenderpass(ResourceHandle handle)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      m_allRes.renderpasses[handle] = VulkanRenderpass();
    }

    void VulkanDevice::createHeap(ResourceHandle handle, HeapDescriptor heapDesc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto&& desc = heapDesc.desc;

      int32_t index, bits;
      unpackInt64(desc.customType, index, bits);

      vk::MemoryAllocateFlagsInfo flags = vk::MemoryAllocateFlagsInfo().setFlags(memoryAddressingFlags());

      vk::MemoryAllocateInfo allocInfo;

      allocInfo = vk::MemoryAllocateInfo()
        .setPNext(&flags)
        .setAllocationSize(desc.sizeInBytes)
        .setMemoryTypeIndex(index);

      auto memory = m_device.allocateMemory(allocInfo);
      VK_CHECK_RESULT(memory);

      m_allRes.heaps[handle] = VulkanHeap(memory.value);
    }

    void VulkanDevice::createBuffer(ResourceHandle handle, ResourceDescriptor& desc)
    {
      /*
      auto vkdesc = fillBufferInfo(desc, m_memoryAddressingEnabled);
      auto buffer = m_device.createBuffer(vkdesc);
      
      vk::MemoryDedicatedAllocateInfoKHR dediInfo;
      dediInfo.setBuffer(buffer);

      auto bufMemReq = m_device.getBufferMemoryRequirements2KHR(vk::BufferMemoryRequirementsInfo2KHR().setBuffer(buffer));
      auto memProp = m_physDevice.getMemoryProperties();
      auto searchProperties = getMemoryProperties(desc.desc.usage);
      auto index = FindProperties(memProp, bufMemReq.memoryRequirements.memoryTypeBits, searchProperties.optimal);

      auto res = m_device.allocateMemory(vk::MemoryAllocateInfo().setPNext(&dediInfo).setAllocationSize(bufMemReq.memoryRequirements.size).setMemoryTypeIndex(index));

      m_device.bindBufferMemory(buffer, res.operator VkDeviceMemory(), 0);

      VulkanBufferState state{};
      state.queueIndex = m_mainQueueIndex;
      m_allRes.buf[handle] = VulkanBuffer(buffer, std::make_shared<VulkanBufferState>(state));
      std::weak_ptr<Garbage> weak = m_trash;
      return std::shared_ptr<VulkanBuffer>(new VulkanBuffer(buffer, std::make_shared<VulkanBufferState>(state)),
          [weak, dev = m_device, mem = res.operator VkDeviceMemory()](VulkanBuffer* ptr)
      {
          if (auto trash = weak.lock())
          {
              trash->buffers.emplace_back(ptr->native());
              trash->heaps.emplace_back(mem);
          }
          else
          {
              dev.destroyBuffer(ptr->native());
              dev.freeMemory(mem);
          }
          delete ptr;
      });
      */
     /*
      VulkanBufferState state{};
      state.queueIndex = m_mainQueueIndex;
      //std::weak_ptr<Garbage> weak = m_trash;
      /*
      return std::shared_ptr<VulkanBuffer>(new VulkanBuffer(buffer, std::make_shared<VulkanBufferState>(state)),
        [weak, dev = m_device](VulkanBuffer* ptr)
      {
        if (auto trash = weak.lock())
        {
          trash->buffers.emplace_back(ptr->native());
        }
        else
        {
          dev.destroyBuffer(ptr->native());
        }
        delete ptr;
      });
      */
    }

    void VulkanDevice::createBuffer(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto vkdesc = fillBufferInfo(desc, m_memoryAddressingEnabled, m_limits.minStorageBufferOffsetAlignment);
      auto buffer = m_device.createBuffer(vkdesc);
      VK_CHECK_RESULT(buffer);
      auto& native = m_allRes.heaps[allocation.heap.handle];
      vk::DeviceSize size = allocation.allocation.block.offset;
      auto emptyVal = m_device.getBufferMemoryRequirements(buffer.value); // Only to silence the debug layers since this actually has been done already
      VK_CHECK_RESULT_RAW(m_device.bindBufferMemory(buffer.value, native.native(), size));

      VulkanBufferState state{};
      state.queueIndex = m_mainQueueIndex;
      m_allRes.buf[handle] = VulkanBuffer(buffer.value);
    }

    void VulkanDevice::createBufferView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& resDesc, ShaderViewDescriptor& viewDesc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto& native = m_allRes.buf[buffer];

      auto& desc = resDesc.desc;

      uint elementSize = desc.stride;
      FormatType format = viewDesc.m_format;
      if (format == FormatType::Unknown)
        format = desc.format;
      else {
        elementSize = formatSizeInfo(format).pixelSize;
      }
      if (resDesc.desc.structured){
        if (elementSize % m_limits.minStorageBufferOffsetAlignment != 0)
          elementSize += (m_limits.minStorageBufferOffsetAlignment - elementSize%m_limits.minStorageBufferOffsetAlignment);
      }
      auto elementCount = viewDesc.m_elementCount;
      if (elementCount == -1)
      {
        elementCount = desc.width;
        if (viewDesc.m_format != FormatType::Unknown)
        {
          elementCount = elementCount / elementSize;
        }
        elementCount = elementCount - viewDesc.m_firstElement;
      }

      auto maxRange = elementCount * elementSize;

      vk::DescriptorType type = vk::DescriptorType::eStorageBuffer;
      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        if (format != FormatType::Unknown && format != FormatType::Raw32)
        {
          type = vk::DescriptorType::eUniformTexelBuffer;
        }
      }
      else if (viewDesc.m_viewType == ResourceShaderType::ReadWrite)
      {
        if (format != FormatType::Unknown && format != FormatType::Raw32)
        {
          type = vk::DescriptorType::eStorageTexelBuffer;
        }
      }
      auto offsetBytes = viewDesc.m_firstElement * elementSize;



      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(native.native())
        .setOffset(offsetBytes)
        .setRange(maxRange);

      if (handle.type == ViewResourceType::BufferSRV) {
        if (type == vk::DescriptorType::eUniformTexelBuffer) {
          auto viewRes = m_device.createBufferView(vk::BufferViewCreateInfo()
            .setBuffer(native.native())
            .setFormat(formatToVkFormat(format).view)
            .setOffset(offsetBytes)
            .setRange(maxRange));
          VK_CHECK_RESULT(viewRes);
          m_allRes.bufSRV[handle] = VulkanBufferView(viewRes.value, type, offsetBytes);
        }
        else {
          m_allRes.bufSRV[handle] = VulkanBufferView(info, type, offsetBytes);
        }
      }
      if (handle.type == ViewResourceType::RaytracingAccelerationStructure){
        HIGAN_ASSERT(false, "unimplemented");
      }
      if (handle.type == ViewResourceType::BufferUAV) {
        if (type == vk::DescriptorType::eStorageTexelBuffer) {
          auto viewRes = m_device.createBufferView(vk::BufferViewCreateInfo()
            .setBuffer(native.native())
            .setFormat(formatToVkFormat(format).view)
            .setOffset(offsetBytes)
            .setRange(maxRange));
          VK_CHECK_RESULT(viewRes);
          m_allRes.bufUAV[handle] = VulkanBufferView(viewRes.value, type, offsetBytes);
        }
        else {
          m_allRes.bufUAV[handle] = VulkanBufferView(info, type, offsetBytes);
        }
      }
      if (handle.type == ViewResourceType::BufferIBV) {
        vk::IndexType type = (format == FormatType::Uint16) ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
        m_allRes.bufIBV[handle] = VulkanBufferView(native.native(), type, offsetBytes);
      }
    }

    void VulkanDevice::createTexture(ResourceHandle handle, ResourceDescriptor& desc)
    {
      /*
      auto vkdesc = fillImageInfo(desc);
      auto image = m_device.createImage(vkdesc);
      m_device.getImageMemoryRequirements(image); // Only to silence the debug layers

      vk::MemoryDedicatedAllocateInfoKHR dediInfo;
      dediInfo.setImage(image);

      auto bufMemReq = m_device.getImageMemoryRequirements2KHR(vk::ImageMemoryRequirementsInfo2KHR().setImage(image));
      auto memProp = m_physDevice.getMemoryProperties();
      auto searchProperties = getMemoryProperties(desc.desc.usage);
      auto index = FindProperties(memProp, bufMemReq.memoryRequirements.memoryTypeBits, searchProperties.optimal);

      auto res = m_device.allocateMemory(vk::MemoryAllocateInfo().setPNext(&dediInfo).setAllocationSize(bufMemReq.memoryRequirements.size).setMemoryTypeIndex(index));

      m_device.bindImageMemory(image, res.operator VkDeviceMemory(), 0);

      vector<TextureStateFlags> state;
      for (uint32_t slice = 0; slice < vkdesc.arrayLayers; ++slice)
      {
          for (uint32_t mip = 0; mip < vkdesc.mipLevels; ++mip)
          {
              state.emplace_back(TextureStateFlags(vk::AccessFlagBits(0), vk::ImageLayout::eUndefined, m_mainQueueIndex));
          }
      }
      std::weak_ptr<Garbage> weak = m_trash;
      return std::shared_ptr<VulkanTexture>(new VulkanTexture(image, std::make_shared<VulkanTextureState>(VulkanTextureState{ state })),
          [weak, dev = m_device, mem = res.operator VkDeviceMemory()](VulkanTexture* ptr)
      {
          if (auto trash = weak.lock())
          {
              trash->textures.emplace_back(ptr->native());
              trash->heaps.emplace_back(mem);
          }
          else
          {
              dev.destroyImage(ptr->native());
              dev.freeMemory(mem);
          }
          delete ptr;
      });  */
      /* 
      vector<TextureStateFlags> state;
      for (uint32_t slice = 0; slice < vkdesc.arrayLayers; ++slice)
      {
        for (uint32_t mip = 0; mip < vkdesc.mipLevels; ++mip)
        {
          state.emplace_back(TextureStateFlags(vk::AccessFlagBits(0), vk::ImageLayout::eUndefined, m_mainQueueIndex));
        }
      }
      /*
      std::weak_ptr<Garbage> weak = m_trash;
      return std::shared_ptr<VulkanTexture>(new VulkanTexture(image, std::make_shared<VulkanTextureState>(VulkanTextureState{ state })),
        [weak, dev = m_device](VulkanTexture* ptr)
      {
        if (auto trash = weak.lock())
        {
          trash->textures.emplace_back(ptr->native());
        }
        else
        {
          dev.destroyImage(ptr->native());
        }
        delete ptr;
      });
      */
    }

    void VulkanDevice::createTexture(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto vkdesc = fillImageInfo(desc);
      auto image = m_device.createImage(vkdesc);
      VK_CHECK_RESULT(image);
      auto& native = m_allRes.heaps[allocation.heap.handle];
      vk::DeviceSize size = allocation.allocation.block.offset;
      auto emptyVal = m_device.getImageMemoryRequirements(image.value); // Only to silence the debug layers, we've already done this with same description.
      HIGAN_ASSERT(size%allocation.allocation.alignment == 0, "hmm, wtf?");
      VK_CHECK_RESULT_RAW(m_device.bindImageMemory(image.value, native.native(), size));

      m_allRes.tex[handle] = VulkanTexture(image.value, desc);
    }

    void VulkanDevice::createTextureView(ViewResourceHandle handle, ResourceHandle texture, ResourceDescriptor& texDesc, ShaderViewDescriptor& viewDesc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto& native = m_allRes.tex[texture];
      auto desc = texDesc.desc;

      bool isArray = desc.arraySize > 1;

      vk::ImageViewType ivt;
      switch (desc.dimension)
      {
      case FormatDimension::Texture1D:
        ivt = vk::ImageViewType::e1D;
        if (isArray)
          ivt = vk::ImageViewType::e1DArray;
        break;
      case FormatDimension::Texture3D:
        ivt = vk::ImageViewType::e3D;
        break;
      case FormatDimension::TextureCube:
        ivt = vk::ImageViewType::eCube;
        if (isArray)
          ivt = vk::ImageViewType::eCubeArray;
        break;
      case FormatDimension::Texture2D:
      default:
        ivt = vk::ImageViewType::e2D;
        if (isArray)
          ivt = vk::ImageViewType::e2DArray;
        break;
      }

      // TODO: verify swizzle, fine for now. expect problems with backbuffer BGRA format...
      vk::ComponentMapping cm = vk::ComponentMapping()
        .setA(vk::ComponentSwizzle::eIdentity)
        .setR(vk::ComponentSwizzle::eIdentity)
        .setG(vk::ComponentSwizzle::eIdentity)
        .setB(vk::ComponentSwizzle::eIdentity);

      FormatType format = viewDesc.m_format;
      if (format == FormatType::Unknown)
        format = desc.format;

      vk::ImageAspectFlags imgFlags;

      if (formatIsDepth(format))
      {
        imgFlags |= vk::ImageAspectFlagBits::eDepth;
        if (formatIsStencil(format))
          imgFlags |= vk::ImageAspectFlagBits::eStencil;
        // TODO: support stencil format, no such FormatType yet.
      }
      else
      {
        imgFlags |= vk::ImageAspectFlagBits::eColor;
      }

      decltype(VK_REMAINING_MIP_LEVELS) mips = (viewDesc.m_viewType == ResourceShaderType::ReadOnly) ? texDesc.desc.miplevels - viewDesc.m_mostDetailedMip : 1;
      if (viewDesc.m_mipLevels != -1)
      {
        mips = viewDesc.m_mipLevels;
      }

      decltype(VK_REMAINING_ARRAY_LAYERS) arraySize = (viewDesc.m_arraySize != -1) ? viewDesc.m_arraySize : texDesc.desc.arraySize - viewDesc.m_arraySlice;

      vk::ImageSubresourceRange subResourceRange = vk::ImageSubresourceRange()
        .setAspectMask(imgFlags)
        .setBaseArrayLayer(viewDesc.m_arraySlice)
        .setBaseMipLevel(viewDesc.m_mostDetailedMip)
        .setLevelCount(mips)
        .setLayerCount(arraySize);


      vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo()
        .setViewType(ivt)
        .setFormat(formatToVkFormat(format).view)
        .setImage(native.native())
        .setComponents(cm)
        .setSubresourceRange(subResourceRange);

      auto view = m_device.createImageView(viewInfo);
      VK_CHECK_RESULT(view);

      vk::DescriptorImageInfo info = vk::DescriptorImageInfo()
        .setImageLayout(vk::ImageLayout::eGeneral) // TODO: layouts
        .setImageView(view.value);


      SubresourceRange range{};
      range.mipOffset = static_cast<int16_t>(subResourceRange.baseMipLevel);
      range.mipLevels = static_cast<int16_t>(subResourceRange.levelCount);
      range.sliceOffset = static_cast<int16_t>(subResourceRange.baseArrayLayer);
      range.arraySize = static_cast<int16_t>(subResourceRange.layerCount);

      vk::DescriptorType imageType = vk::DescriptorType::eInputAttachment;
      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        imageType = vk::DescriptorType::eSampledImage;
        info = info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        m_allRes.texSRV[handle] = VulkanTextureView(view.value, info, formatToVkFormat(format).view, imageType, range, imgFlags);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::ReadWrite)
      {
        imageType = vk::DescriptorType::eStorageImage;
        m_allRes.texUAV[handle] = VulkanTextureView(view.value, info, formatToVkFormat(format).view, imageType, range, imgFlags);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::DepthStencil)
      {
        imageType = vk::DescriptorType::eInputAttachment;
        m_allRes.texDSV[handle] = VulkanTextureView(view.value, info, formatToVkFormat(format).view, imageType, range, imgFlags);
      }
      else if (viewDesc.m_viewType == ResourceShaderType::RenderTarget)
      {
        imageType = vk::DescriptorType::eInputAttachment;
        m_allRes.texRTV[handle] = VulkanTextureView(view.value, info, formatToVkFormat(format).view, imageType, range, imgFlags);
      }
    }
    
    void VulkanDevice::createShaderArgumentsLayout(ResourceHandle handle, ShaderArgumentsLayoutDescriptor& desc)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto bindings = gatherSetLayoutBindings(desc, vk::ShaderStageFlagBits::eAll);
      vk::DescriptorBindingFlagsEXT partial = vk::DescriptorBindingFlagBitsEXT::ePartiallyBound
      | vk::DescriptorBindingFlagBitsEXT::eVariableDescriptorCount
      | vk::DescriptorBindingFlagBitsEXT::eUpdateUnusedWhilePending;
      vector<vk::DescriptorBindingFlagsEXT> flags(bindings.size(), vk::DescriptorBindingFlagsEXT());
      flags.back() = partial;
      vk::DescriptorSetLayoutBindingFlagsCreateInfo extflags = vk::DescriptorSetLayoutBindingFlagsCreateInfo()
        .setBindingCount(bindings.size())
        .setPBindingFlags(flags.data());
      vk::DescriptorSetLayoutCreateInfo info = vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(static_cast<uint32_t>(bindings.size()))
        .setPBindings(bindings.data());

      if (!desc.bindless.name.empty()) {
        info = info.setPNext(&extflags);
        info = info.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPoolEXT);
      }

      auto setlayout = m_device.createDescriptorSetLayout(info);
      VK_CHECK_RESULT(setlayout);
      setDebugUtilsObjectNameEXT(setlayout.value, "some layout");
      m_allRes.shaArgsLayouts[handle] = VulkanShaderArgumentsLayout(setlayout.value);
    }

    void VulkanDevice::createShaderArguments(ResourceHandle handle, ShaderArgumentsDescriptor& binding)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto desclayout = allResources().shaArgsLayouts[binding.layout()].native();
      
      vk::DescriptorSetVariableDescriptorCountAllocateInfo extInfo{};
      uint32_t val = static_cast<uint32_t>(binding.bBindless().size());
      extInfo = extInfo.setDescriptorCounts(val);
      auto res = vk::DescriptorSetAllocateInfo()
        .setDescriptorSetCount(1)
        .setPSetLayouts(&desclayout);
      
      if (!binding.bBindlessDesc().name.empty()) {
        res = res.setPNext(&extInfo);
      }

      auto set = m_descriptors->allocate(native(), res)[0];
      setDebugUtilsObjectNameEXT(set, binding.name().c_str());
      m_allRes.shaArgs[handle] = VulkanShaderArguments(set);
      //m_allocatedSets.push_back(set);
      m_descriptorSetsInUse++;

      vector<vk::WriteDescriptorSet> writeDescriptors;
/*
      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(block.buffer())
        .setOffset(0)
        .setRange(block.size());
        */

      int index = 0;

      auto& resources = binding.bResources();
      auto& descriptions = binding.bDescriptors();
      for (auto&& descriptor : resources)
      {
        const auto& binddesc = descriptions[index];
        vk::WriteDescriptorSet writeSet = vk::WriteDescriptorSet()
        .setDstSet(set)
        .setDescriptorCount(1)
        .setDstBinding(index++);
        switch (descriptor.type)
        {
          case ViewResourceType::BufferSRV:
          {
            auto& desc = allResources().bufSRV[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.type);
            if (binddesc.type == ShaderResourceType::Buffer)
            {
              writeSet = writeSet.setDescriptorType(vk::DescriptorType::eUniformTexelBuffer);
              writeSet = writeSet.setPTexelBufferView(&desc.view);
            }
            else
            {
              writeSet = writeSet.setDescriptorType(vk::DescriptorType::eStorageBuffer);
              writeSet = writeSet.setPBufferInfo(&desc.bufferInfo);
            }
            break;
          }
          case ViewResourceType::BufferUAV:
          {
            auto& desc = allResources().bufUAV[descriptor].native();
            if (binddesc.type == ShaderResourceType::Buffer)
            {
              writeSet = writeSet.setDescriptorType(vk::DescriptorType::eStorageTexelBuffer);
              writeSet = writeSet.setPTexelBufferView(&desc.view);
            }
            else
            {
              writeSet = writeSet.setDescriptorType(vk::DescriptorType::eStorageBuffer);
              writeSet = writeSet.setPBufferInfo(&desc.bufferInfo);
            }
            break;
          }
          case ViewResourceType::RaytracingAccelerationStructure:
          {
            HIGAN_ASSERT(false, "unimplemented");
            break;
          }
          case ViewResourceType::TextureSRV:
          {
            auto& desc = allResources().texSRV[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.viewType)
                        .setPImageInfo(&desc.info);
            break;
          }
          case ViewResourceType::TextureUAV:
          {
            auto& desc = allResources().texUAV[descriptor].native();
            writeSet = writeSet.setDescriptorType(desc.viewType)
                        .setPImageInfo(&desc.info);
            break;
          }
          case ViewResourceType::DynamicBufferSRV:
          {
            auto& desc = allResources().dynBuf[descriptor].native();
            if (binddesc.type == ShaderResourceType::Buffer)
            {
              writeSet = writeSet.setPTexelBufferView(&desc.texelView);
              writeSet = writeSet.setDescriptorType(vk::DescriptorType::eUniformTexelBuffer);
            }
            else
            {
              writeSet = writeSet.setPBufferInfo(&desc.bufferInfo);
              writeSet = writeSet.setDescriptorType(vk::DescriptorType::eStorageBuffer);
            }
            break;
          }
          default:
          {
            // figure null resource here..
            vk::DescriptorType imagedt = vk::DescriptorType::eSampledImage;
            if (!binddesc.readonly)
              imagedt = vk::DescriptorType::eStorageImage;
            // figure out format
            auto format = FormatType::Unorm8RGBA;
            if (binddesc.elementType == ShaderElementType::Unsigned)
              format = FormatType::Uint8RGBA;
            else if (binddesc.elementType == ShaderElementType::Integer)
              format = FormatType::Int8RGBA;
            auto& ni = nullImages(format);
            switch(binddesc.type)
            {
              case ShaderResourceType::Buffer:
              {
                writeSet = writeSet.setDescriptorType(vk::DescriptorType::eUniformTexelBuffer);
                writeSet = writeSet.setPTexelBufferView(&nullBuffers(format).texel);
                break;
              }
              case ShaderResourceType::ByteAddressBuffer:
              case ShaderResourceType::StructuredBuffer:
              {
                writeSet = writeSet.setDescriptorType(vk::DescriptorType::eStorageBuffer);
                writeSet = writeSet.setPBufferInfo(&nullBuffers(format).ssbo);
                break;
              }
              case ShaderResourceType::Texture1D:
              {
                writeSet = writeSet.setDescriptorType(imagedt);
                writeSet = writeSet.setPImageInfo(&ni.desc1D);
                break;
              }
              case ShaderResourceType::Texture1DArray:
              {
                writeSet = writeSet.setDescriptorType(imagedt);
                writeSet = writeSet.setPImageInfo(&ni.desc1DArray);
                break;
              }
              case ShaderResourceType::Texture2D:
              {
                writeSet = writeSet.setDescriptorType(imagedt);
                writeSet = writeSet.setPImageInfo(&ni.desc2D);
                break;
              }
              case ShaderResourceType::Texture2DArray:
              {
                writeSet = writeSet.setDescriptorType(imagedt);
                writeSet = writeSet.setPImageInfo(&ni.desc2DArray);
                break;
              }
              case ShaderResourceType::Texture3D:
              {
                writeSet = writeSet.setDescriptorType(imagedt);
                writeSet = writeSet.setPImageInfo(&ni.desc3D);
                break;
              }
              case ShaderResourceType::TextureCube:
              {
                writeSet = writeSet.setDescriptorType(imagedt);
                writeSet = writeSet.setPImageInfo(&ni.descCube);
                break;
              }
              case ShaderResourceType::TextureCubeArray:
              {
                writeSet = writeSet.setDescriptorType(imagedt);
                writeSet = writeSet.setPImageInfo(&ni.descCubeArray);
                break;
              }
              default:
              {
                HIGAN_ERROR("unhandled null resource");
              }
            }
          }
        }

        writeDescriptors.emplace_back(writeSet);
      }
      HIGAN_ASSERT(descriptions.size() == writeDescriptors.size(), "size should match");
      vector<vk::DescriptorImageInfo> bindlessInfos;
      if (!binding.bBindlessDesc().name.empty())
      {
        HIGAN_ASSERT(binding.bBindlessDesc().readonly && binding.bBindlessDesc().type == ShaderResourceType::Texture2D, "read only tex supported");
        for (auto&& it : binding.bBindless())
        {
          if (it.type == ViewResourceType::Unknown) {
            break;
            //bindlessInfos.push_back(nullTex2dd);
          }
          else
            bindlessInfos.push_back(allResources().texSRV[it].native().info);
        }
        vk::WriteDescriptorSet writeSet = vk::WriteDescriptorSet()
          .setDstSet(set)
          .setDescriptorCount(bindlessInfos.size())
          .setPImageInfo(bindlessInfos.data())
          .setDescriptorType(vk::DescriptorType::eSampledImage)
          .setDstBinding(index++);
        if (!bindlessInfos.empty())
          writeDescriptors.emplace_back(writeSet);
        HIGAN_ASSERT(!writeDescriptors.empty(), "wtf!");
        vk::ArrayProxy<const vk::WriteDescriptorSet> writes(writeDescriptors.size(), writeDescriptors.data());
        m_device.updateDescriptorSets(writes, {});
        return;
      }
      HIGAN_ASSERT(!writeDescriptors.empty(), "wtf!");
      vk::ArrayProxy<const vk::WriteDescriptorSet> writes(writeDescriptors.size(), writeDescriptors.data());
      m_device.updateDescriptorSets(writes, {});
    }

    /*
    VulkanConstantBuffer VulkanDevice::allocateConstants(MemView<uint8_t> bytes)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto upload = m_constantAllocators->allocate(bytes.size());
      HIGAN_ASSERT(upload, "Halp");

      memcpy(upload.data(), bytes.data(), bytes.size());

      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(upload.buffer())
        .setOffset(0)
        .setRange(bytes.size());

      return VulkanConstantBuffer(info, upload);
    }*/

    size_t VulkanDevice::availableDynamicMemory() {
      return m_dynamicUpload->size();
    }

    void VulkanDevice::dynamic(ViewResourceHandle handle, MemView<uint8_t> dataRange, FormatType desiredFormat)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto alignment = formatSizeInfo(desiredFormat).pixelSize * m_limits.minTexelBufferOffsetAlignment;
      backend::VkUploadBlock upload;
      {
        HIGAN_CPU_BRACKET("dunamicUpload allocate");
        upload = m_dynamicUpload->allocate(dataRange.size(), alignment);
      }
      HIGAN_ASSERT(upload, "Halp");
      HIGAN_ASSERT(upload.block.offset % m_limits.minTexelBufferOffsetAlignment == 0, "fail, mintexelbufferoffsetalignment is %d", m_limits.minTexelBufferOffsetAlignment);
      {
        std::string memcpySize = std::string("memcpy ") + std::to_string(dataRange.size_bytes()) +  std::string(" bytes");
        
        HIGAN_CPU_BRACKET(memcpySize.c_str());
        memcpy(upload.data(), dataRange.data(), dataRange.size());
      }

      auto format = formatToVkFormat(desiredFormat).view;
      //auto stride = formatSizeInfo(desiredFormat).pixelSize;

      auto type = vk::DescriptorType::eUniformTexelBuffer;

      vk::BufferViewCreateInfo desc = vk::BufferViewCreateInfo()
        .setBuffer(upload.buffer())
        .setFormat(format)
        .setOffset(upload.offset())
        .setRange(dataRange.size());

      vk::BufferView rview;

      {
        HIGAN_CPU_BRACKET("createBufferView");
        auto view = m_device.createBufferView(desc);
        VK_CHECK_RESULT(view);
        rview = view.value;
      }

      vk::IndexType indextype = vk::IndexType::eUint16;
      if (formatBitDepth(desiredFormat) == 16)
      {
        indextype = vk::IndexType::eUint16;
      }
      else if(formatBitDepth(desiredFormat) == 32)
      {
        indextype = vk::IndexType::eUint32;
      }
      
      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(upload.buffer())
        .setOffset(upload.offset())
        .setRange(dataRange.size());

      m_allRes.dynBuf[handle] = VulkanDynamicBufferView(upload.buffer(), rview, info, upload, indextype);
    }

    void VulkanDevice::dynamic(ViewResourceHandle handle, MemView<uint8_t> dataRange, unsigned stride)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto upload = m_dynamicUpload->allocate(dataRange.size(), stride);
      HIGAN_ASSERT(upload, "Halp");
      memcpy(upload.data(), dataRange.data(), dataRange.size());

      auto type = vk::DescriptorType::eStorageBuffer;
      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(upload.buffer())
        .setOffset(upload.block.offset)
        .setRange(upload.block.size);

      HIGAN_ASSERT(upload.block.offset % stride == 0, "oh no");
      m_allRes.dynBuf[handle] = VulkanDynamicBufferView(upload.buffer(), info, upload);
      // will be collected promtly
    }

    void VulkanDevice::dynamicImage(ViewResourceHandle handle, MemView<uint8_t> dataRange, unsigned rowPitch)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto upload = m_dynamicUpload->allocate(dataRange.size(), rowPitch);
      HIGAN_ASSERT(upload, "Halp");
      memcpy(upload.data(), dataRange.data(), dataRange.size());

      auto type = vk::DescriptorType::eStorageBuffer;
      vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo()
        .setBuffer(upload.buffer())
        .setOffset(upload.block.offset)
        .setRange(upload.block.size);

      HIGAN_ASSERT(upload.block.offset % rowPitch == 0, "oh no");
      m_allRes.dynBuf[handle] = VulkanDynamicBufferView(upload.buffer(), info, upload, rowPitch);
    }

    void VulkanDevice::readbackBuffer(ResourceHandle readback, size_t bytes)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      ResourceDescriptor desc = ResourceDescriptor()
        .setElementsCount(bytes)
        .setUsage(ResourceUsage::Readback);

      auto vkdesc = fillBufferInfo(desc, m_memoryAddressingEnabled, m_limits.minStorageBufferOffsetAlignment);
      auto buffer = m_device.createBuffer(vkdesc);
      VK_CHECK_RESULT(buffer);
      
      vk::MemoryAllocateFlagsInfo flags = vk::MemoryAllocateFlagsInfo().setFlags(memoryAddressingFlags());
      vk::MemoryDedicatedAllocateInfoKHR dediInfo;
      dediInfo = dediInfo.setBuffer(buffer.value).setPNext(&flags);

      //auto bufMemReq = m_device.getBufferMemoryRequirements(buffer.value);
      auto chain = m_device.getBufferMemoryRequirements2KHR<vk::MemoryRequirements2, vk::MemoryDedicatedRequirementsKHR>(vk::BufferMemoryRequirementsInfo2KHR().setBuffer(buffer.value), m_dynamicDispatch);

      auto dedReq = chain.get<vk::MemoryDedicatedRequirementsKHR>();

      /* bonus info
      bool dedicatedAllocation =
              (dedReq.requiresDedicatedAllocation != VK_FALSE) ||
              (dedReq.prefersDedicatedAllocation != VK_FALSE);
      HIGAN_ASSERT(dedicatedAllocation, "We really want dedicated here");
      */

      auto bufMemReq = chain.get<vk::MemoryRequirements2>();

      auto memProp = m_physDevice.getMemoryProperties();
      auto searchProperties = getMemoryProperties(desc.desc.usage);
      auto index = FindProperties(memProp, bufMemReq.memoryRequirements.memoryTypeBits, searchProperties.optimal);

      auto res = m_device.allocateMemory(vk::MemoryAllocateInfo().setPNext(&dediInfo).setAllocationSize(bufMemReq.memoryRequirements.size).setMemoryTypeIndex(index));
      VK_CHECK_RESULT(res);

      auto res2 = m_device.bindBufferMemory(buffer.value, res.value, 0);
      VK_CHECK_RESULT_RAW(res2);

      m_allRes.rbBuf[readback] = VulkanReadback(res.value, buffer.value, 0, bytes);
    }

    MemView<uint8_t> VulkanDevice::mapReadback(ResourceHandle readback)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto& vrb = m_allRes.rbBuf[readback];
      auto res = m_device.mapMemory(vrb.memory(), vrb.offset(), vrb.size());
      VK_CHECK_RESULT(res);
      uint8_t* ptr = reinterpret_cast<uint8_t*>(res.value);
      return MemView<uint8_t>(ptr, vrb.size());
    }

    void VulkanDevice::unmapReadback(ResourceHandle readback)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto& vrb = m_allRes.rbBuf[readback];
      m_device.unmapMemory(vrb.memory());
    }

    VulkanCommandList VulkanDevice::createCommandBuffer(int queueIndex)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer))
        .setQueueFamilyIndex(queueIndex);
      auto pool = m_device.createCommandPool(poolInfo);
      VK_CHECK_RESULT(pool);
      auto buffer = m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
        .setCommandBufferCount(1)
        .setCommandPool(pool.value)
        .setLevel(vk::CommandBufferLevel::ePrimary));
      VK_CHECK_RESULT(buffer);

      auto ptr = std::shared_ptr<vk::CommandPool>(new vk::CommandPool(pool.value), [&](vk::CommandPool* ptr)
      {
        HIGAN_CPU_BRACKET("kill command pool");
        m_device.destroyCommandPool(*ptr);
        delete ptr;
      });

      return VulkanCommandList(buffer.value[0], ptr);
    }

    std::shared_ptr<CommandBufferImpl> VulkanDevice::createDMAList()
    {
      HIGAN_CPU_FUNCTION_SCOPE();

      auto list = m_copyListPool.allocate();
      //resetListNative(*list);
      return std::shared_ptr<VulkanCommandBuffer>(new VulkanCommandBuffer(list, m_constantAllocators, m_readbackPool.allocate(), m_dmaQueryPoolPool.allocate(), m_shaderDebugBuffer.native(), m_dynamicDispatch),
        [&](VulkanCommandBuffer* buffer)
      {
        HIGAN_CPU_BRACKET("destroy command buffer");
        {
          HIGAN_CPU_BRACKET("free constants");
          for (auto&& constant : buffer->freeableConstants())
          {
            m_constantAllocators->release(constant);
          }
        }
        delete buffer;
      });
    }
    std::shared_ptr<CommandBufferImpl> VulkanDevice::createComputeList()
    {
      HIGAN_CPU_FUNCTION_SCOPE();

      auto list = m_computeListPool.allocate();
      //resetListNative(*list);
      return std::shared_ptr<VulkanCommandBuffer>(new VulkanCommandBuffer(list, m_constantAllocators, m_readbackPool.allocate(), m_computeQueryPoolPool.allocate(), m_shaderDebugBuffer.native(), m_dynamicDispatch),
        [&](VulkanCommandBuffer* buffer)
      {
        HIGAN_CPU_BRACKET("destroy command buffer");
        {
          HIGAN_CPU_BRACKET("free constants");
          for (auto&& constant : buffer->freeableConstants())
          {
            m_constantAllocators->release(constant);
          }
        }
        {
          HIGAN_CPU_BRACKET("free pipelines");
          for (auto&& oldPipe : buffer->oldPipelines())
          {
            if (oldPipe)
              m_device.destroyPipeline(oldPipe);
          }
        }
        delete buffer;
      });
    }
    std::shared_ptr<CommandBufferImpl> VulkanDevice::createGraphicsList()
    {
      HIGAN_CPU_FUNCTION_SCOPE();

      auto list = m_graphicsListPool.allocate();
      //resetListNative(*list);
      return std::shared_ptr<VulkanCommandBuffer>(new VulkanCommandBuffer(list, m_constantAllocators, m_readbackPool.allocate(), m_queryPoolPool.allocate(), m_shaderDebugBuffer.native(), m_dynamicDispatch),
        [&](VulkanCommandBuffer* buffer)
      {
        HIGAN_CPU_BRACKET("destroy command buffer");
        {
          HIGAN_CPU_BRACKET("free constants");
          for (auto&& constant : buffer->freeableConstants())
          {
            m_constantAllocators->release(constant);
          }
        }
        {
          HIGAN_CPU_BRACKET("free pipelines");
          for (auto&& oldPipe : buffer->oldPipelines())
          {
            if (oldPipe)
              m_device.destroyPipeline(oldPipe);
          }
        }
        delete buffer;
      });
    }

    void VulkanDevice::resetListNative(VulkanCommandList list)
    {
      m_device.resetCommandPool(list.pool(), vk::CommandPoolResetFlagBits::eReleaseResources);
    }

    std::shared_ptr<SemaphoreImpl> VulkanDevice::createSemaphore()
    {
      return m_semaphores.allocate();
    }

    std::shared_ptr<TimelineSemaphoreImpl> VulkanDevice::createTimelineSemaphore()
    {
      vk::SemaphoreTypeCreateInfo timelineCreateInfo{};
      timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
      timelineCreateInfo.initialValue = 0;

      vk::SemaphoreCreateInfo createInfo{};
      createInfo.pNext = &timelineCreateInfo;
      //createInfo.flags = 0;

      auto result = m_device.createSemaphore(createInfo);
      VK_CHECK_RESULT(result);
      return std::make_shared<VulkanTimelineSemaphore>(std::shared_ptr<vk::Semaphore>(new vk::Semaphore(result.value), [device = m_device](vk::Semaphore* ptr)
      {
        device.destroySemaphore(*ptr);
        delete ptr;
      }));
    }

    std::shared_ptr<FenceImpl> VulkanDevice::createFence()
    {
      return m_fences.allocate();
    }

    std::shared_ptr<ConstantsAllocator> VulkanDevice::createConstantsAllocator(size_t size)
    {
      return nullptr;
    }

    void VulkanDevice::submitToQueue(vk::Queue queue,
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<TimelineSemaphoreInfo> waitTimelines,
      MemView<TimelineSemaphoreInfo> signalTimelines,
      std::optional<std::shared_ptr<FenceImpl>>   fence)
    {
      vector<vk::Semaphore> waitList;
      vector<uint64_t> waitValues;
      vector<vk::CommandBuffer> bufferList;
      vector<vk::Semaphore> signalList;
      vector<uint64_t> signalValues;
      for (auto&& sema : wait)
      {
        auto native = std::static_pointer_cast<VulkanSemaphore>(sema);
        waitList.emplace_back(native->native());
        waitValues.emplace_back(1);
      }
      for (auto&& sema : waitTimelines)
      {
        auto native = static_cast<VulkanTimelineSemaphore*>(sema.semaphore);
        waitList.emplace_back(native->native());
        waitValues.emplace_back(sema.value);
      }
      for (auto&& buffer : lists)
      {
        auto native = std::static_pointer_cast<VulkanCommandBuffer>(buffer);
        bufferList.emplace_back(native->list());
      }

      for (auto&& sema : signal)
      {
        auto native = std::static_pointer_cast<VulkanSemaphore>(sema);
        signalList.emplace_back(native->native());
        signalValues.emplace_back(1);
      }

      for (auto&& sema : signalTimelines)
      {
        auto native = static_cast<VulkanTimelineSemaphore*>(sema.semaphore);
        signalList.emplace_back(native->native());
        signalValues.emplace_back(sema.value);
      }

      vk::TimelineSemaphoreSubmitInfo tsinfo = vk::TimelineSemaphoreSubmitInfo()
          .setPWaitSemaphoreValues(waitValues.data())
          .setWaitSemaphoreValueCount(waitValues.size())
          .setPSignalSemaphoreValues(signalValues.data())
          .setSignalSemaphoreValueCount(signalValues.size());

      vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eBottomOfPipe;

      vector<vk::PipelineStageFlags> semaphoreWaitStages;
      for (int i = 0; i < waitList.size(); ++i)
        semaphoreWaitStages.push_back(waitMask);

      auto info = vk::SubmitInfo()
        .setPNext(&tsinfo)
        .setPWaitDstStageMask(semaphoreWaitStages.data())
        .setCommandBufferCount(static_cast<uint32_t>(bufferList.size()))
        .setPCommandBuffers(bufferList.data())
        .setWaitSemaphoreCount(static_cast<uint32_t>(waitList.size()))
        .setPWaitSemaphores(waitList.data())
        .setSignalSemaphoreCount(static_cast<uint32_t>(signalList.size()))
        .setPSignalSemaphores(signalList.data());
      if (fence)
      {
        auto native = std::static_pointer_cast<VulkanFence>(*fence);
        {
          // TODO: is this ok to do always?
          m_device.resetFences({native->native()});
        }
        auto res = queue.submit({info}, native->native());
        VK_CHECK_RESULT_RAW(res);
      }
      else
      {
        auto res = queue.submit({info}, nullptr);
        VK_CHECK_RESULT_RAW(res);
      }
    }

    void VulkanDevice::submitDMA(
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<TimelineSemaphoreInfo> waitTimelines,
      MemView<TimelineSemaphoreInfo> signalTimelines,
      std::optional<std::shared_ptr<FenceImpl>>         fence)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::Queue target = m_copyQueue;
      if (m_singleQueue)
      {
        target = m_mainQueue;
      }
      submitToQueue(target,
        std::forward<decltype(lists)>(lists),
        std::forward<decltype(wait)>(wait),
        std::forward<decltype(signal)>(signal),
        std::forward<decltype(waitTimelines)>(waitTimelines),
        std::forward<decltype(signalTimelines)>(signalTimelines),
        std::forward<decltype(fence)>(fence));
    }

    void VulkanDevice::submitCompute(
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<TimelineSemaphoreInfo> waitTimelines,
      MemView<TimelineSemaphoreInfo> signalTimelines,
      std::optional<std::shared_ptr<FenceImpl>>         fence)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::Queue target = m_computeQueue;
      if (m_singleQueue)
      {
        target = m_mainQueue;
      }
      submitToQueue(target,
        std::forward<decltype(lists)>(lists),
        std::forward<decltype(wait)>(wait),
        std::forward<decltype(signal)>(signal),
        std::forward<decltype(waitTimelines)>(waitTimelines),
        std::forward<decltype(signalTimelines)>(signalTimelines),
        std::forward<decltype(fence)>(fence));
    }

    void VulkanDevice::submitGraphics(
      MemView<std::shared_ptr<CommandBufferImpl>> lists,
      MemView<std::shared_ptr<SemaphoreImpl>>     wait,
      MemView<std::shared_ptr<SemaphoreImpl>>     signal,
      MemView<TimelineSemaphoreInfo> waitTimelines,
      MemView<TimelineSemaphoreInfo> signalTimelines,
      std::optional<std::shared_ptr<FenceImpl>>   fence)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      submitToQueue(m_mainQueue,
        std::forward<decltype(lists)>(lists),
        std::forward<decltype(wait)>(wait),
        std::forward<decltype(signal)>(signal),
        std::forward<decltype(waitTimelines)>(waitTimelines),
        std::forward<decltype(signalTimelines)>(signalTimelines),
        std::forward<decltype(fence)>(fence));
    }

    void VulkanDevice::waitFence(std::shared_ptr<FenceImpl> fence)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto native = std::static_pointer_cast<VulkanFence>(fence);
      HIGAN_ASSERT(native->native(), "expected non null handle");
      vk::Fence vkfence = native->native();
      vk::ArrayProxy<const vk::Fence> proxy(vkfence);
      auto res = m_device.waitForFences(proxy, 1, (std::numeric_limits<int64_t>::max)());
      HIGAN_ASSERT(res == vk::Result::eSuccess, "uups");
    }

    bool VulkanDevice::checkFence(std::shared_ptr<FenceImpl> fence)
    {
      auto native = std::static_pointer_cast<VulkanFence>(fence);
      auto status = m_device.getFenceStatus(native->native());

      return status == vk::Result::eSuccess;
    }

    uint64_t VulkanDevice::completedValue(std::shared_ptr<backend::TimelineSemaphoreImpl> tlSema)
    {
      auto native = std::static_pointer_cast<VulkanTimelineSemaphore>(tlSema);
      uint64_t value;
      VK_CHECK_RESULT_RAW(m_device.getSemaphoreCounterValueKHR(native->native(), &value, m_dynamicDispatch));
      return value;
    }

    void VulkanDevice::waitTimeline(std::shared_ptr<backend::TimelineSemaphoreImpl> tlSema, uint64_t value)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto native = std::static_pointer_cast<VulkanTimelineSemaphore>(tlSema);
      auto sema = native->native();
      vk::SemaphoreWaitInfoKHR wi = vk::SemaphoreWaitInfoKHR()
        .setSemaphoreCount(1)
        .setPSemaphores(&sema)
        .setPValues(&value);
      VK_CHECK_RESULT_RAW(m_device.waitSemaphoresKHR(wi, UINT64_MAX, m_dynamicDispatch));
    }

    void VulkanDevice::present(std::shared_ptr<prototypes::SwapchainImpl> swapchain, std::shared_ptr<SemaphoreImpl> renderingFinished, int indexb)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      auto native = std::static_pointer_cast<VulkanSwapchain>(swapchain);
      uint32_t index = static_cast<uint32_t>(indexb); // turns out that index from swapchain isnt threadsafe, better be explicit.
      vk::Semaphore renderFinish = nullptr;
      uint32_t semaphoreCount = 0;
      vk::SwapchainKHR swap = native->native();
      if (renderingFinished)
      {
        auto sema = std::static_pointer_cast<VulkanSemaphore>(renderingFinished);
        renderFinish = sema->native();
        semaphoreCount = 1;
      }

      auto result = m_mainQueue.presentKHR(vk::PresentInfoKHR()
        .setSwapchainCount(1)
        .setPSwapchains(&swap)
        .setPImageIndices(&index)
        .setWaitSemaphoreCount(semaphoreCount)
        .setPWaitSemaphores(&renderFinish));
      
      if (result != vk::Result::eSuccess)
      {
        native->setOutOfDate(true);
      }
    }
    
    std::shared_ptr<TimelineSemaphoreImpl> VulkanDevice::createSharedSemaphore()
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::ExportSemaphoreWin32HandleInfoKHR exportInfoHandle = vk::ExportSemaphoreWin32HandleInfoKHR().setDwAccess(GENERIC_ALL);

      vk::ExportSemaphoreCreateInfo exportSema = vk::ExportSemaphoreCreateInfo()
        .setPNext(&exportInfoHandle)
        .setHandleTypes(vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32);

      vk::SemaphoreTypeCreateInfo timelineCreateInfo = vk::SemaphoreTypeCreateInfo();
      timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
      timelineCreateInfo.initialValue = 0;

      auto interopSemaphoreInfo = m_physDevice.getExternalSemaphoreProperties(vk::PhysicalDeviceExternalSemaphoreInfo()
        .setPNext(&timelineCreateInfo)
        .setHandleType(vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32));

      HIGAN_LOGi("compatibleHandleTypes %s\n", vk::to_string(interopSemaphoreInfo.compatibleHandleTypes).c_str());
      HIGAN_LOGi("exportFromImportedHandleTypes %s\n", vk::to_string(interopSemaphoreInfo.exportFromImportedHandleTypes).c_str());
      HIGAN_LOGi("externalSemaphoreFeatures %s\n", vk::to_string(interopSemaphoreInfo.externalSemaphoreFeatures).c_str());
      interopSemaphoreInfo = m_physDevice.getExternalSemaphoreProperties(vk::PhysicalDeviceExternalSemaphoreInfo()
        .setPNext(&timelineCreateInfo)
        .setHandleType(vk::ExternalSemaphoreHandleTypeFlagBits::eD3D12Fence));

      HIGAN_LOGi("compatibleHandleTypes %s\n", vk::to_string(interopSemaphoreInfo.compatibleHandleTypes).c_str());
      HIGAN_LOGi("exportFromImportedHandleTypes %s\n", vk::to_string(interopSemaphoreInfo.exportFromImportedHandleTypes).c_str());
      HIGAN_LOGi("externalSemaphoreFeatures %s\n", vk::to_string(interopSemaphoreInfo.externalSemaphoreFeatures).c_str());

      timelineCreateInfo = timelineCreateInfo.setPNext(&exportSema);

      auto fence = m_device.createSemaphore(vk::SemaphoreCreateInfo().setPNext(&timelineCreateInfo), nullptr, m_dynamicDispatch);
      VK_CHECK_RESULT(fence);
      return std::make_shared<VulkanTimelineSemaphore>(VulkanTimelineSemaphore(std::shared_ptr<vk::Semaphore>(new vk::Semaphore(fence.value), [device = m_device](vk::Semaphore* ptr)
      {
        device.destroySemaphore(*ptr);
        delete ptr;
      })));
#else
      return nullptr;
#endif
    };

    std::shared_ptr<SharedHandle> VulkanDevice::openSharedHandle(std::shared_ptr<TimelineSemaphoreImpl> shared)
    {
#ifdef HIGANBANA_PLATFORM_WINDOWS
      HIGAN_CPU_FUNCTION_SCOPE();
      auto native = std::static_pointer_cast<VulkanTimelineSemaphore>(shared)->native();

      vk::SemaphoreGetWin32HandleInfoKHR getHandle = vk::SemaphoreGetWin32HandleInfoKHR().setSemaphore(native).setHandleType(vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32);
      auto handle = m_device.getSemaphoreWin32HandleKHR(getHandle, m_dynamicDispatch);
      VK_CHECK_RESULT(handle);
      return std::shared_ptr<SharedHandle>(new SharedHandle{GraphicsApi::Vulkan, handle.value }, [](SharedHandle* ptr)
      {
        CloseHandle(ptr->handle);
        delete ptr;
      });
#else
      return nullptr;
#endif
    };
    std::shared_ptr<SharedHandle> VulkanDevice::openSharedHandle(HeapAllocation heapAllocation)
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
      HIGAN_CPU_FUNCTION_SCOPE();
      auto& native = m_allRes.heaps[heapAllocation.heap.handle];

      auto handle = m_device.getMemoryWin32HandleKHR(vk::MemoryGetWin32HandleInfoKHR()
        .setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32)
        .setMemory(native.native()), m_dynamicDispatch);
      VK_CHECK_RESULT(handle);

      return std::shared_ptr<SharedHandle>(new SharedHandle{GraphicsApi::Vulkan, handle.value }, [](SharedHandle* ptr)
      {
        CloseHandle(ptr->handle);
        delete ptr;
      });
#else
      return nullptr;
#endif
    }
    std::shared_ptr<SharedHandle> VulkanDevice::openSharedHandle(ResourceHandle handle) { return nullptr; };
    std::shared_ptr<TimelineSemaphoreImpl> VulkanDevice::createSemaphoreFromHandle(std::shared_ptr<SharedHandle> shared)
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::Semaphore sema;
      vk::SemaphoreTypeCreateInfo timelineCreateInfo = vk::SemaphoreTypeCreateInfo();
      timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
      timelineCreateInfo.initialValue = 0;
      auto fence = m_device.createSemaphore(vk::SemaphoreCreateInfo().setPNext(&timelineCreateInfo), nullptr, m_dynamicDispatch);
      VK_CHECK_RESULT(fence);
      sema = fence.value;
      auto importInfo = vk::ImportSemaphoreWin32HandleInfoKHR()
      .setHandle(shared->handle)
      .setSemaphore(sema)
      //.setFlags(vk::SemaphoreImportFlagBits::eTemporary)
      .setHandleType(vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32);
      auto res = m_device.importSemaphoreWin32HandleKHR(importInfo, m_dynamicDispatch);
      VK_CHECK_RESULT_RAW(res);
      return std::make_shared<VulkanTimelineSemaphore>(VulkanTimelineSemaphore(std::shared_ptr<vk::Semaphore>(new vk::Semaphore(sema), [device = m_device](vk::Semaphore* ptr)
      {
        device.destroySemaphore(*ptr);
        delete ptr;
      })));
#else
      return nullptr;
#endif
    };
    
    void VulkanDevice::createHeapFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared)
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::ImportMemoryWin32HandleInfoKHR info = vk::ImportMemoryWin32HandleInfoKHR()
      .setHandle(shared->handle);
      if (shared->api == GraphicsApi::DX12) // DX12 resource
      {
        info = info.setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Heap);
      }
      else // vulkan resource
      {
        info = info.setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32);
      }
      vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
      .setPNext(&info)
      .setAllocationSize(shared->heapsize);

      auto memory = m_device.allocateMemory(allocInfo);
      VK_CHECK_RESULT(memory);

      m_allRes.heaps[handle] = VulkanHeap(memory.value);
#else
#endif
    }

    void VulkanDevice::createBufferFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared, HeapAllocation allocation, ResourceDescriptor& descriptor)
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::ImportMemoryWin32HandleInfoKHR info = vk::ImportMemoryWin32HandleInfoKHR()
      .setHandle(shared->handle);
      if (shared->api == GraphicsApi::DX12) // DX12 resource
      {
        info = info.setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Heap);
      }
      else // vulkan resource
      {
        info = info.setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32);
      }
      vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
      .setPNext(&info)
      .setAllocationSize(shared->heapsize);

      auto memory = m_device.allocateMemory(allocInfo);
      VK_CHECK_RESULT(memory);

      m_allRes.heaps[allocation.heap.handle] = VulkanHeap(memory.value);

      createBuffer(handle, allocation, descriptor);
#endif
    }
    void VulkanDevice::createTextureFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared, ResourceDescriptor& descriptor)
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
      HIGAN_CPU_FUNCTION_SCOPE();
      vk::ExternalMemoryImageCreateInfo extInfo;
      extInfo = extInfo.setHandleTypes(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource);
      auto vkdesc = fillImageInfo(descriptor);
      vkdesc = vkdesc.setPNext(&extInfo);
      auto image = m_device.createImage(vkdesc);
      VK_CHECK_RESULT(image);


      vk::PhysicalDeviceExternalImageFormatInfo extImageInfo = vk::PhysicalDeviceExternalImageFormatInfo()
      .setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource);

      auto formatInfo = vk::PhysicalDeviceImageFormatInfo2()
        .setPNext(&extImageInfo)
        .setFlags(vkdesc.flags)
        .setFormat(vkdesc.format)
        .setTiling(vkdesc.tiling)
        .setType(vkdesc.imageType)
        .setUsage(vkdesc.usage);

      auto checkSupport = m_physDevice.getImageFormatProperties2(formatInfo);
      VK_CHECK_RESULT(checkSupport);
      {
        auto r = checkSupport.value;
      }

      vk::ImportMemoryWin32HandleInfoKHR importInfo = vk::ImportMemoryWin32HandleInfoKHR()
      .setHandle(shared->handle);
      if (shared->api == GraphicsApi::DX12)
      {
        importInfo = importInfo.setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource);
      }
      else
      {
        importInfo = importInfo.setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32);
      }

      vk::MemoryDedicatedAllocateInfo dedInfo = vk::MemoryDedicatedAllocateInfo()
//        .setPNext(&importInfo)
        .setImage(image.value);

      importInfo = importInfo.setPNext(&dedInfo);

      auto req = m_device.getImageMemoryRequirements(image.value); 

      vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setPNext(&importInfo)
        .setAllocationSize(req.size);

      auto memory = m_device.allocateMemory(allocInfo);
      VK_CHECK_RESULT(memory);
      //auto& native = m_allRes.heaps[allocation.heap.handle];
      //vk::DeviceSize size = 0;
      //auto req = m_device.getImageMemoryRequirements(image.value); // Only to silence the debug layers, we've already done this with same description.
      VK_CHECK_RESULT_RAW(m_device.bindImageMemory(image.value, memory.value, 0));
      m_allRes.tex[handle] = VulkanTexture(image.value, descriptor, memory.value);
#endif
    }

    desc::RaytracingASPreBuildInfo VulkanDevice::accelerationStructurePrebuildInfo(const desc::RaytracingAccelerationStructureInputs& desc) {
      vector<uint> primitiveCounts;
      vector<vk::AccelerationStructureGeometryTrianglesDataKHR> triangles;
      for (auto&& geo : desc.desc.triangles) {
        vk::AccelerationStructureGeometryTrianglesDataKHR data;
        auto& desc = geo;
        if (desc.indexBuffer.id != ResourceHandle::InvalidId) {
          auto& nat = allResources().buf[desc.indexBuffer];
          auto bufferaddr = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(nat.native()), m_dynamicDispatch);
          data = data.setIndexData(bufferaddr + desc.indexByteOffset);
        }
        if (desc.transformBuffer.id != ResourceHandle::InvalidId) {
          auto& nat = allResources().buf[desc.transformBuffer];
          auto bufferaddr = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(nat.native()), m_dynamicDispatch);
          data = data.setTransformData(bufferaddr + desc.transformByteOffset);
        }
        if (desc.indexBuffer.id != ResourceHandle::InvalidId) {
          auto& nat = allResources().buf[desc.indexBuffer];
          auto bufferaddr = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(nat.native()), m_dynamicDispatch);
          data = data.setVertexData(bufferaddr + desc.vertexByteOffset)
                    .setVertexStride(desc.vertexStride);
        }
        vk::IndexType indextype = vk::IndexType::eUint16;
        if (formatBitDepth(desc.indexFormat) == 16)
        {
          indextype = vk::IndexType::eUint16;
        }
        else if(formatBitDepth(desc.indexFormat) == 32)
        {
          indextype = vk::IndexType::eUint32;
        }
        data = data.setIndexType(indextype)
          .setMaxVertex(desc.vertexCount)
          .setVertexFormat(formatToVkFormat(desc.vertexFormat).view)
          .setVertexStride(desc.vertexStride);
        triangles.emplace_back(data);
        primitiveCounts.push_back(desc.indexCount/3);
      }

      vector<vk::AccelerationStructureGeometryKHR> geometries;
      for (auto&& tris : triangles) {
        geometries.push_back(vk::AccelerationStructureGeometryKHR()
          .setGeometry(vk::AccelerationStructureGeometryDataKHR().setTriangles(tris))
          .setGeometryType(vk::GeometryTypeKHR::eTriangles)
          .setFlags(vk::GeometryFlagBitsKHR::eOpaque));
      }

      vk::AccelerationStructureBuildGeometryInfoKHR info = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setGeometries(geometries)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
      vk::AccelerationStructureBuildSizesInfoKHR sizes = m_device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, info, primitiveCounts, m_dynamicDispatch);
      desc::RaytracingASPreBuildInfo ret{};
      ret.ResultDataMaxSizeInBytes = sizes.accelerationStructureSize;
      ret.ScratchDataSizeInBytes = sizes.buildScratchSize;
      ret.UpdateScratchDataSizeInBytes = sizes.updateScratchSize;
      return ret;
    }
  }
}