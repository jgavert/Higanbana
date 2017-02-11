#include "vkresources.hpp"

#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DeviceImpl::DeviceImpl(
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

  }
}