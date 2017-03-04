#include "vkresources.hpp"
#include "util/formats.hpp"
#include "faze/src/new_gfx/common/graphicssurface.hpp"
#include "core/src/system/bitpacking.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {

    void printMemoryTypeInfos(vk::PhysicalDeviceMemoryProperties prop)
    {
      auto checkFlagSet = [](vk::MemoryType& type, vk::MemoryPropertyFlagBits flag)
      {
        return (type.propertyFlags & flag) == flag;
      };
      F_ILOG("Graphics/Memory", "heapCount %u memTypeCount %u", prop.memoryHeapCount, prop.memoryTypeCount);
      for (int i = 0; i < static_cast<int>(prop.memoryTypeCount); ++i)
      {
        auto memType = prop.memoryTypes[i];
        F_ILOG("Graphics/Memory", "propertyFlags %u", memType.propertyFlags);
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
          }
        }
      }
    }

    VulkanDevice::VulkanDevice(
      vk::Device device,
      vk::PhysicalDevice physDev,
      FileSystem& fs,
      std::vector<vk::QueueFamilyProperties> queues,
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
        //m_internalUniversalQueue = m_device.getQueue(0, 0);
      }
      if (m_freeQueueIndexes.universal.size() > 0
        && m_freeQueueIndexes.graphics.size() > 0)
      {
        F_ERROR("abort mission. Too many variations of queues.");
      }

      m_computeQueues = !m_freeQueueIndexes.compute.empty();
      m_dmaQueues = !m_freeQueueIndexes.dma.empty();
      m_graphicQueues = !m_freeQueueIndexes.graphics.empty();

      if (m_singleQueue)
      {
        m_mainQueue = m_device.getQueue(0, 0);
        m_mainQueueIndex = 0;
      }
      else
      {
        if (!m_freeQueueIndexes.universal.empty())
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.universalIndex;
          uint32_t queueId = m_freeQueueIndexes.universal.back();
          m_freeQueueIndexes.universal.pop_back();
          m_mainQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_mainQueueIndex = queueFamilyIndex;
        }
        else
        {
          uint32_t queueFamilyIndex = m_freeQueueIndexes.graphicsIndex;
          uint32_t queueId = m_freeQueueIndexes.graphics.back();
          m_freeQueueIndexes.graphics.pop_back();
          m_mainQueue = m_device.getQueue(queueFamilyIndex, queueId);
          m_mainQueueIndex = queueFamilyIndex;
        }
      }
      //auto memProp = m_physDevice.getMemoryProperties();
      //printMemoryTypeInfos(memProp);
      /* if VK_KHR_display is a things, use the below. For now, Borderless fullscreen!
      {
          auto displayProperties = m_physDevice.getDisplayPropertiesKHR();
          F_SLOG("Vulkan", "Trying to query available displays...\n");
          for (auto&& prop : displayProperties)
          {
              F_LOG("%s Physical Dimensions: %dx%d Resolution: %dx%d %s %s\n",
                  prop.displayName, prop.physicalDimensions.width, prop.physicalDimensions.height,
                  prop.physicalResolution.width, prop.physicalResolution.height,
                  ((prop.planeReorderPossible) ? " Plane Reorder possible " : ""),
                  ((prop.persistentContent) ? " Persistent Content " : ""));
          }
      }*/
    }

    VulkanDevice::~VulkanDevice()
    {
      m_device.waitIdle();
      m_device.destroy();
    }

    std::shared_ptr<prototypes::SwapchainImpl> VulkanDevice::createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int buffers)
    {
      auto natSurface = std::static_pointer_cast<VulkanGraphicsSurface>(surface.native());
      auto surfaceCap = m_physDevice.getSurfaceCapabilitiesKHR(natSurface->native());
      F_SLOG("Graphics/Surface", "surface details\n");
      F_SLOG("Graphics/Surface", "min image Count: %d\n", surfaceCap.minImageCount);
      F_SLOG("Graphics/Surface", "current res %dx%d\n", surfaceCap.currentExtent.width, surfaceCap.currentExtent.height);
      F_SLOG("Graphics/Surface", "min res %dx%d\n", surfaceCap.minImageExtent.width, surfaceCap.minImageExtent.height);
      F_SLOG("Graphics/Surface", "max res %dx%d\n", surfaceCap.maxImageExtent.width, surfaceCap.maxImageExtent.height);

      auto formats = m_physDevice.getSurfaceFormatsKHR(natSurface->native());

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
        F_SLOG("Graphics/Surface", "format: %s\n", vk::to_string(fmt.format).c_str());
      }

      if (!found)
      {
        F_ASSERT(hadBackup, "uh oh, backup format wasn't supported either.");
        wantedFormat = backupFormat;
        format = FormatType::Unorm8x4_Bgr;
      }

      auto asd = m_physDevice.getSurfacePresentModesKHR(natSurface->native());

      vk::PresentModeKHR khrmode;
      switch (mode)
      {
      case PresentMode::Mailbox:
        khrmode = vk::PresentModeKHR::eMailbox;
        break;
      case PresentMode::Fifo:
        khrmode = vk::PresentModeKHR::eFifo;
        break;
      case PresentMode::FifoRelaxed:
        khrmode = vk::PresentModeKHR::eFifoRelaxed;
        break;
      case PresentMode::Immediate:
      default:
        khrmode = vk::PresentModeKHR::eImmediate;
        break;
      }

      bool hadChosenMode = false;
      for (auto&& fmt : asd)
      {
        if (fmt == vk::PresentModeKHR::eImmediate)
          F_SLOG("Graphics/AvailablePresentModes", "Immediate\n");
        if (fmt == vk::PresentModeKHR::eMailbox)
          F_SLOG("Graphics/AvailablePresentModes", "Mailbox\n");
        if (fmt == vk::PresentModeKHR::eFifo)
          F_SLOG("Graphics/AvailablePresentModes", "Fifo\n");
        if (fmt == vk::PresentModeKHR::eFifoRelaxed)
          F_SLOG("Graphics/AvailablePresentModes", "FifoRelaxed\n");
        if (fmt == khrmode)
          hadChosenMode = true;
      }
      if (!hadChosenMode)
      {
        khrmode = vk::PresentModeKHR::eFifo; // guaranteed by spec
        mode = PresentMode::Fifo;
      }

      auto extent = surfaceCap.currentExtent;
      if (extent.height == 0)
      {
        extent.height = surfaceCap.minImageExtent.height;
      }
      if (extent.width == 0)
      {
        extent.width = surfaceCap.minImageExtent.width;
      }

      if (!m_physDevice.getSurfaceSupportKHR(m_mainQueueIndex, natSurface->native()))
      {
        F_ASSERT(false, "Was not supported.");
      }
      int minImageCount = std::max(static_cast<int>(surfaceCap.minImageCount), buffers);
      F_SLOG("Vulkan", "creating swapchain to %ux%u, buffers %d\n", extent.width, extent.height, minImageCount);

      vk::SwapchainCreateInfoKHR info = vk::SwapchainCreateInfoKHR()
        .setSurface(natSurface->native())
        .setMinImageCount(minImageCount)
        .setImageFormat(wantedFormat)
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent(surfaceCap.currentExtent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)  // linear to here
        .setImageSharingMode(vk::SharingMode::eExclusive)
        //	.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eInherit)
        //	.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eInherit)
        .setPresentMode(khrmode)
        .setClipped(false);

      auto swapchain = m_device.createSwapchainKHR(info);
      auto sc = std::make_shared<VulkanSwapchain>(swapchain, *natSurface);
      
      sc->setBufferMetadata(surfaceCap.currentExtent.width, surfaceCap.currentExtent.height, minImageCount, format, mode);
      return sc;
    }

    void VulkanDevice::adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> swapchain, PresentMode mode, FormatType format, int bufferCount)
    {
      auto natSwapchain = std::static_pointer_cast<VulkanSwapchain>(swapchain);
      auto& natSurface = natSwapchain->surface();
      auto oldSwapchain = natSwapchain->native();

      format = (format == FormatType::Unknown) ? natSwapchain->getDesc().format : format;
      bufferCount = (bufferCount == -1) ? natSwapchain->getDesc().buffers : bufferCount;
      mode = (mode == PresentMode::Unknown) ? natSwapchain->getDesc().mode : mode;

      auto surfaceCap = m_physDevice.getSurfaceCapabilitiesKHR(natSurface.native());

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

      if (!m_physDevice.getSurfaceSupportKHR(m_mainQueueIndex, natSurface.native()))
      {
        F_ASSERT(false, "Was not supported.");
      }
      vk::PresentModeKHR khrmode;
      switch (mode)
      {
      case PresentMode::Mailbox:
        khrmode = vk::PresentModeKHR::eMailbox;
        break;
      case PresentMode::Fifo:
        khrmode = vk::PresentModeKHR::eFifo;
        break;
      case PresentMode::FifoRelaxed:
        khrmode = vk::PresentModeKHR::eFifoRelaxed;
        break;
      case PresentMode::Immediate:
      default:
        khrmode = vk::PresentModeKHR::eImmediate;
        break;
      }

      vk::SwapchainCreateInfoKHR info = vk::SwapchainCreateInfoKHR()
        .setSurface(natSurface.native())
        .setMinImageCount(minImageCount)
        .setImageFormat(formatToVkFormat(format).storage)
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent(surfaceCap.currentExtent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)  // linear to here
        .setImageSharingMode(vk::SharingMode::eExclusive)
        //	.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eInherit)
        //	.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eInherit)
        .setPresentMode(khrmode)
        .setClipped(false)
        .setOldSwapchain(oldSwapchain);

      natSwapchain->setSwapchain(m_device.createSwapchainKHR(info));

      m_device.destroySwapchainKHR(oldSwapchain);
      //F_SLOG("Vulkan", "adjusting swapchain to %ux%u\n", surfaceCap.currentExtent.width, surfaceCap.currentExtent.height);
      natSwapchain->setBufferMetadata(surfaceCap.currentExtent.width, surfaceCap.currentExtent.height, minImageCount, format, mode);
    }

    void VulkanDevice::destroySwapchain(std::shared_ptr<prototypes::SwapchainImpl> swapchain)
    {
      auto native = std::static_pointer_cast<VulkanSwapchain>(swapchain);
      m_device.destroySwapchainKHR(native->native());
    }

    vector<std::shared_ptr<prototypes::TextureImpl>> VulkanDevice::getSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc)
    {
      auto native = std::static_pointer_cast<VulkanSwapchain>(sc);
      auto images = m_device.getSwapchainImagesKHR(native->native());
      vector<std::shared_ptr<prototypes::TextureImpl>> textures;
      textures.resize(images.size());
      for (int i = 0; i < static_cast<int>(images.size()); ++i)
      {
        textures[i] = std::make_shared<VulkanTexture>(images[i], false);
      }
      return textures;
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

    struct MemoryPropertySearch
    {
      vk::MemoryPropertyFlags optimal;
      vk::MemoryPropertyFlags def;
    };

    MemoryPropertySearch getMemoryProperties(ResourceUsage usage)
    {
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

    MemoryRequirements VulkanDevice::getReqs(ResourceDescriptor desc)
    {
      MemoryRequirements reqs{};
      vk::MemoryRequirements requirements;
      if (desc.desc.dimension == FormatDimension::Buffer)
      {
        auto buffer = m_device.createBuffer(fillBufferInfo(desc));
        requirements = m_device.getBufferMemoryRequirements(buffer);
        m_device.destroyBuffer(buffer);
      }
      else
      {
        auto image = m_device.createImage(fillImageInfo(desc));
        requirements = m_device.getImageMemoryRequirements(image);
        m_device.destroyImage(image);
      }
      auto memProp = m_physDevice.getMemoryProperties();
      auto searchProperties = getMemoryProperties(desc.desc.usage);
      auto index = FindProperties(memProp, requirements.memoryTypeBits, searchProperties.optimal);
      F_ASSERT(index != -1, "Couldn't find optimal memory... maybe try default :D?"); // searchProperties.def
      auto packed = packInt64(index, 0);
      //int32_t v1, v2;
      //unpackInt64(packed, v1, v2);
      //F_ILOG("Vulkan", "Did (unnecessary) packing tricks, packed %d -> %zd -> %d", index, packed, v1);
      reqs.heapType = packed;
      reqs.alignment = requirements.alignment;
      reqs.bytes = requirements.size;

      if (reqs.alignment < 128)
      {
          reqs.alignment = ((128 + reqs.alignment - 1) / reqs.alignment) * reqs.alignment;
      }

      return reqs;
    }

    GpuHeap VulkanDevice::createHeap(HeapDescriptor heapDesc)
    {
      auto&& desc = heapDesc.desc;

      int32_t index, bits;
      unpackInt64(desc.customType, index, bits);

      vk::MemoryAllocateInfo allocInfo;

      allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(desc.sizeInBytes)
        .setMemoryTypeIndex(index);

      auto memory = m_device.allocateMemory(allocInfo);

      return GpuHeap(std::make_shared<VulkanHeap>(memory), std::move(heapDesc));
    }

    void VulkanDevice::destroyHeap(GpuHeap heap)
    {
      auto native = std::static_pointer_cast<VulkanHeap>(heap.impl);
      m_device.freeMemory(native->native());
    }

    std::shared_ptr<prototypes::BufferImpl> VulkanDevice::createBuffer(HeapAllocation allocation, ResourceDescriptor& desc)
    {
      auto vkdesc = fillBufferInfo(desc);
      auto buffer = m_device.createBuffer(vkdesc);
      auto native = std::static_pointer_cast<VulkanHeap>(allocation.heap.impl);
      vk::DeviceSize size = allocation.allocation.block.offset;
      m_device.getBufferMemoryRequirements(buffer); // Only to silence the debug layers
      m_device.bindBufferMemory(buffer, native->native(), size);
      return std::make_shared<VulkanBuffer>(buffer);
    }

    void VulkanDevice::destroyBuffer(std::shared_ptr<prototypes::BufferImpl> buffer)
    {
      auto native = std::static_pointer_cast<VulkanBuffer>(buffer);
      m_device.destroyBuffer(native->native());
    }

    std::shared_ptr<prototypes::BufferViewImpl> VulkanDevice::createBufferView(std::shared_ptr<prototypes::BufferImpl> buffer, ResourceDescriptor& resDesc, ShaderViewDescriptor& viewDesc)
    {
      auto native = std::static_pointer_cast<VulkanBuffer>(buffer);

      auto& desc = resDesc.desc;

      auto elementSize = desc.stride; // TODO: actually 0 most of the time. FIX SIZE FROM FORMAT.
      auto sizeInElements = desc.width;
      auto firstElement = viewDesc.m_firstElement * elementSize;
      auto maxRange = viewDesc.m_elementCount * elementSize;
      if (viewDesc.m_elementCount <= 0)
        maxRange = elementSize * sizeInElements; // VK_WHOLE_SIZE

      vk::DescriptorType type = vk::DescriptorType::eStorageBuffer; // TODO: figure out sane buffers for vulkan...
      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        type = vk::DescriptorType::eStorageBuffer;
      }
      else if (viewDesc.m_viewType == ResourceShaderType::ReadWrite)
      {
        type = vk::DescriptorType::eStorageBuffer;
      }
      return std::make_shared<VulkanBufferView>(vk::DescriptorBufferInfo()
        .setBuffer(native->native())
        .setOffset(firstElement)
        .setRange(maxRange)
        , type);
    }

    void VulkanDevice::destroyBufferView(std::shared_ptr<prototypes::BufferViewImpl>)
    {
      // craak craak
    }

    std::shared_ptr<prototypes::TextureImpl> VulkanDevice::createTexture(HeapAllocation allocation, ResourceDescriptor& desc)
    {
      auto vkdesc = fillImageInfo(desc);
      auto image = m_device.createImage(vkdesc);
      auto native = std::static_pointer_cast<VulkanHeap>(allocation.heap.impl);
      vk::DeviceSize size = allocation.allocation.block.offset;
      auto req = m_device.getImageMemoryRequirements(image); // Only to silence the debug layers
      m_device.bindImageMemory(image, native->native(), size);
      return std::make_shared<VulkanTexture>(image);
    }

    void VulkanDevice::destroyTexture(std::shared_ptr<prototypes::TextureImpl> texture)
    {
      auto native = std::static_pointer_cast<VulkanTexture>(texture);
      if (native->canRelease())
        m_device.destroyImage(native->native());
    }

    std::shared_ptr<prototypes::TextureViewImpl> VulkanDevice::createTextureView(
      std::shared_ptr<prototypes::TextureImpl> texture, ResourceDescriptor& texDesc, ShaderViewDescriptor& viewDesc)
    {
      auto native = std::static_pointer_cast<VulkanTexture>(texture);
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

      vk::ImageAspectFlags imgFlags;

      if (viewDesc.m_viewType == ResourceShaderType::DepthStencil)
      {
        imgFlags |= vk::ImageAspectFlagBits::eDepth;
        // TODO: support stencil format, no such FormatType yet.
        //imgFlags |= vk::ImageAspectFlagBits::eStencil;
      }
      else
      {
        imgFlags |= vk::ImageAspectFlagBits::eColor;
      }


      decltype(VK_REMAINING_MIP_LEVELS) mips = (viewDesc.m_viewType == ResourceShaderType::ReadOnly) ? VK_REMAINING_MIP_LEVELS : 1;
      if (viewDesc.m_mipLevels != -1)
      {
        mips = viewDesc.m_mipLevels;
      }

      decltype(VK_REMAINING_ARRAY_LAYERS) arraySize = (viewDesc.m_arraySize != -1) ? viewDesc.m_arraySize : VK_REMAINING_ARRAY_LAYERS;

      vk::ImageSubresourceRange subResourceRange = vk::ImageSubresourceRange()
        .setAspectMask(imgFlags)
        .setBaseArrayLayer(viewDesc.m_arraySlice)
        .setBaseMipLevel(viewDesc.m_mostDetailedMip)
        .setLevelCount(mips)
        .setLayerCount(arraySize);

      FormatType format = viewDesc.m_format;
      if (format == FormatType::Unknown)
        format = desc.format;

      vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo()
        .setViewType(ivt)
        .setFormat(formatToVkFormat(format).view)
        .setImage(native->native())
        .setComponents(cm)
        .setSubresourceRange(subResourceRange);

      auto view = m_device.createImageView(viewInfo);

      vk::DescriptorImageInfo info = vk::DescriptorImageInfo()
        .setImageLayout(vk::ImageLayout::eGeneral) // TODO: layouts
        .setImageView(view);

      vk::DescriptorType imageType = vk::DescriptorType::eInputAttachment;
      if (viewDesc.m_viewType == ResourceShaderType::ReadOnly)
      {
        imageType = vk::DescriptorType::eSampledImage; // uhh, so simple
      }
      else if (viewDesc.m_viewType == ResourceShaderType::ReadWrite)
      {
        imageType = vk::DescriptorType::eStorageImage; // ah, so simple
      }
      else if (viewDesc.m_viewType == ResourceShaderType::DepthStencil)
      {
        imageType = vk::DescriptorType::eInputAttachment; // Cannot be anything else \o/
      }
      else if (viewDesc.m_viewType == ResourceShaderType::RenderTarget)
      {
        imageType = vk::DescriptorType::eInputAttachment; // Cannot be anything else \o/
      }

      return std::make_shared<VulkanTextureView>(view, info, formatToVkFormat(format).view, imageType, subResourceRange);
    }

    void VulkanDevice::destroyTextureView(std::shared_ptr<prototypes::TextureViewImpl> view)
    {
      auto native = std::static_pointer_cast<VulkanTextureView>(view);
      m_device.destroyImageView(native->native().view);
    }

    void VulkanDevice::submit(backend::IntermediateList&)
    {

    }
  }
}