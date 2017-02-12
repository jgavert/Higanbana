#include "vkresources.hpp"
#include "util/formats.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    VulkanDevice::VulkanDevice(
      vk::Device device,
      vk::PhysicalDevice physDev,
      FileSystem& fs,
      std::vector<vk::QueueFamilyProperties> queues,
      vk::PhysicalDeviceMemoryProperties memProp,
      GpuInfo info,
      bool debugLayer)
      : m_device(device)
      , m_physDevice(physDev)
      , m_debugLayer(debugLayer)
      , m_queues(queues)
      , m_singleQueue(false)
      , m_computeQueues(false)
      , m_dmaQueues(false)
      , m_graphicQueues(false)
      , m_info(info)
      , m_shaders(fs, "../vkShaders", "spirv")
      , m_freeQueueIndexes({})
    {
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
        F_ERROR("wtf, not sane device.");
      }
      else if (totalQueues == 1)
      {
        // single queue =_=, IIIINNNTTTEEEELLL
        m_singleQueue = true;
        // lets just fetch it and copy it for those who need it.
        //auto que = m_device->getQueue(0, 0); // TODO: 0 index is wrong.
        m_internalUniversalQueue = m_device.getQueue(0, 0);
      }
      if (m_freeQueueIndexes.universal.size() > 0
        && m_freeQueueIndexes.graphics.size() > 0)
      {
        F_ERROR("abort mission. Too many variations of queues.");
      }

      m_computeQueues = !m_freeQueueIndexes.compute.empty();
      m_dmaQueues = !m_freeQueueIndexes.dma.empty();
      m_graphicQueues = !m_freeQueueIndexes.graphics.empty();

      for (unsigned i = 0; i < memProp.memoryHeapCount; ++i)
      {
        F_ILOG("Graphics/Memory", "memory heap %u: %.3fGB", i, float(memProp.memoryHeaps[i].size) / 1024.f / 1024.f / 1024.f);
      }

      auto memTypeCount = memProp.memoryTypeCount;
      auto memPtr = memProp.memoryTypes;

      auto checkFlagSet = [](vk::MemoryType& type, vk::MemoryPropertyFlagBits flag)
      {
        return (type.propertyFlags & flag) == flag;
      };
      F_ILOG("Graphics/Memory", "heapCount %u memTypeCount %u", memProp.memoryHeapCount, memTypeCount);
      for (int i = 0; i < static_cast<int>(memTypeCount); ++i)
      {
        // TODO probably bug here with flags.
        auto memType = memPtr[i];
        if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
          F_ILOG("Graphics/Memory", "heap %u type %u was eDeviceLocal", memType.heapIndex, i);
          if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
          {
            F_ILOG("Graphics/Memory", "heap %u type %u was eHostVisible", memType.heapIndex, i);
            if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCoherent))
              F_ILOG("Graphics/Memory", "heap %u type %u was eHostCoherent", memType.heapIndex, i);
            if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
              F_ILOG("Graphics/Memory", "heap %u type %u was eHostCached", memType.heapIndex, i);
            // weird memory only for uma... usually
            if (m_memoryTypes.deviceHostIndex == -1)
              m_memoryTypes.deviceHostIndex = i;
          }
          else
          {
            //F_ILOG("MemoryTypeDebug", "type %u was not eHostVisible", i);
            if (m_memoryTypes.deviceLocalIndex == -1) // always first legit index, otherwise errors.
              m_memoryTypes.deviceLocalIndex = i;
          }
        }
        else if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostVisible))
        {
          F_ILOG("Graphics/Memory", "heap %u type %u was eHostVisible", memType.heapIndex, i);
          if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCoherent))
            F_ILOG("Graphics/Memory", "heap %u type %u was eHostCoherent", memType.heapIndex, i);
          if (checkFlagSet(memType, vk::MemoryPropertyFlagBits::eHostCached))
          {
            F_ILOG("Graphics/Memory", "heap %u type %u was eHostCached", memType.heapIndex, i);
            if (m_memoryTypes.hostCachedIndex == -1)
              m_memoryTypes.hostCachedIndex = i;
          }
          else
          {
            //F_ILOG("MemoryTypeDebug", "type %u was not eHostCached", i);
            if (m_memoryTypes.hostNormalIndex == -1)
              m_memoryTypes.hostNormalIndex = i;
          }
        }
      }
      // validify memorytypes
      if (m_memoryTypes.deviceHostIndex != -1 || ((m_memoryTypes.deviceLocalIndex != -1) || (m_memoryTypes.hostNormalIndex != -1)))
      {
        // normal!
        F_ILOG("Graphics/Memory", "deviceHostIndex(UMA) %d", m_memoryTypes.deviceHostIndex);
        F_ILOG("Graphics/Memory", "deviceLocalIndex %d", m_memoryTypes.deviceLocalIndex);
        F_ILOG("Graphics/Memory", "hostNormalIndex %d", m_memoryTypes.hostNormalIndex);
        F_ILOG("Graphics/Memory", "hostCachedIndex %d", m_memoryTypes.hostCachedIndex);
      }
      else
      {
        F_ERROR("not a sane situation."); // not a sane situation.
      }

      // figure out indexes for default, upload, readback...
    }

    VulkanDevice::~VulkanDevice()
    {
      m_device.waitIdle();
      m_device.destroy();
    }

    vk::BufferCreateInfo VulkanDevice::fillBufferInfo(ResourceDescriptor descriptor)
    {
      auto desc = descriptor.desc;
      auto bufSize = desc.stride*desc.width;
      F_ASSERT(bufSize != 0, "Cannot create zero sized buffers.");
      vk::BufferCreateInfo info = vk::BufferCreateInfo()
        .setSharingMode(vk::SharingMode::eExclusive);

      vk::BufferUsageFlags usageBits;
      if (desc.usage == ResourceUsage::GpuRW)
      {
        if (desc.format != FormatType::Unknown)
        {
          usageBits = vk::BufferUsageFlagBits::eStorageTexelBuffer;
        }
        else
        {
          usageBits = vk::BufferUsageFlagBits::eStorageBuffer;
        }
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

    vk::ImageCreateInfo VulkanDevice::fillImageInfo(ResourceDescriptor descriptor)
    {
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
        F_ASSERT(mipLevels == 1, "DepthStencil doesn't support mips");
        break;
      }
      case ResourceUsage::DepthStencilRW:
      {
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        usage |= vk::ImageUsageFlagBits::eStorage;
        F_ASSERT(mipLevels == 1, "DepthStencil doesn't support mips");
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
        .setMipLevels(desc.miplevels)
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


    void VulkanDevice::waitGpuIdle()
    {
      m_device.waitIdle();
    }

    MemoryRequirements VulkanDevice::getReqs(ResourceDescriptor desc)
    {
      MemoryRequirements reqs{};
      if (desc.desc.dimension == FormatDimension::Buffer)
      {
        auto buffer = m_device.createBuffer(fillBufferInfo(desc));
        auto requirements = m_device.getBufferMemoryRequirements(buffer);
        m_device.destroyBuffer(buffer);
        reqs.alignment = requirements.alignment;
        reqs.bytes = requirements.size;
      }
      else
      {
        auto image = m_device.createImage(fillImageInfo(desc));
        auto requirements = m_device.getImageMemoryRequirements(image);
        m_device.destroyImage(image);
        reqs.alignment = requirements.alignment;
        reqs.bytes = requirements.size;
      }

      return reqs;
    }

    GpuHeap VulkanDevice::createHeap(HeapDescriptor heapDesc)
    {
      auto&& desc = heapDesc.desc;

      vk::MemoryAllocateInfo allocInfo;
      if (m_info.type == DeviceType::IntegratedGpu)
      {
        if (m_memoryTypes.deviceHostIndex != -1)
        {
          allocInfo = vk::MemoryAllocateInfo()
            .setAllocationSize(desc.sizeInBytes)
            .setMemoryTypeIndex(static_cast<uint32_t>(m_memoryTypes.deviceHostIndex));
        }
        else
        {
          F_ERROR("uma but no memory type can be used");
        }
      }
      else
      {
        uint32_t memoryTypeIndex = 0;
        if ((desc.heapType == HeapType::Default) && m_memoryTypes.deviceLocalIndex != -1)
        {
          memoryTypeIndex = m_memoryTypes.deviceLocalIndex;
        }
        else if (desc.heapType == HeapType::Readback && m_memoryTypes.hostNormalIndex != -1)
        {
          memoryTypeIndex = m_memoryTypes.hostNormalIndex;
        }
        else if (desc.heapType == HeapType::Upload && m_memoryTypes.hostCachedIndex != -1)
        {
          memoryTypeIndex = m_memoryTypes.hostCachedIndex;
        }
        else
        {
          F_ERROR("normal device but no valid memory type available");
        }
        allocInfo = vk::MemoryAllocateInfo()
          .setAllocationSize(desc.sizeInBytes)
          .setMemoryTypeIndex(static_cast<uint32_t>(memoryTypeIndex));
      }

      auto memory = m_device.allocateMemory(allocInfo);

      return GpuHeap(std::make_shared<VulkanHeap>(memory), std::move(heapDesc));
    }

    void VulkanDevice::destroyHeap(GpuHeap heap)
    {
      auto native = std::static_pointer_cast<VulkanHeap>(heap.impl);
      m_device.freeMemory(native->native());
    }

    void VulkanDevice::createBuffer(GpuHeap, size_t , ResourceDescriptor )
    {

    }

    void VulkanDevice::createBufferView(ShaderViewDescriptor )
    {

    }
  }
}