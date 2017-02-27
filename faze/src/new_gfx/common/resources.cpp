#include "resources.hpp"
#include "gpudevice.hpp"
#include "implementation.hpp"

namespace faze
{
  namespace backend
  {
    SubsystemData::SubsystemData(GraphicsApi api, const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : impl(std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion))
      , appName(appName)
      , appVersion(appVersion)
      , engineName(engineName)
      , engineVersion(engineVersion)
    {
      switch (api)
      {
#if defined(PLATFORM_WINDOWS)
      case GraphicsApi::DX12:
        impl = std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion);
        break;
#endif
      default:
        impl = std::make_shared<VulkanSubsystem>(appName, appVersion, engineName, engineVersion);
        break;
      }
    }
    std::string SubsystemData::gfxApi() { return impl->gfxApi(); }
    vector<GpuInfo> SubsystemData::availableGpus() { return impl->availableGpus(); }
    GpuDevice SubsystemData::createDevice(FileSystem& fs, GpuInfo gpu) { return impl->createGpuDevice(fs, gpu); }

    // device
    DeviceData::DeviceData(std::shared_ptr<prototypes::DeviceImpl> impl)
      : m_impl(impl)
      , m_bufferTracker(std::make_shared<ResourceTracker<prototypes::BufferImpl>>())
      , m_idGenerator(std::make_shared<std::atomic<int64_t>>())
    {
    }

    DeviceData::~DeviceData()
    {
      waitGpuIdle();
      auto buffers = m_bufferTracker->getResources();
      for (auto&& buffer : buffers)
      {
        m_impl->destroyBuffer(buffer);
      }

      auto bufferAllocations = m_bufferTracker->getAllocations();
      for (auto&& allocation : bufferAllocations)
      {
        m_heaps.release(allocation);
      }

      auto heaps = m_heaps.emptyHeaps();
      for (auto&& heap : heaps)
      {
        m_impl->destroyHeap(heap); 
      }
    }

    void DeviceData::waitGpuIdle() { m_impl->waitGpuIdle(); }
    Buffer DeviceData::createBuffer(ResourceDescriptor desc)
    {
      auto memRes = m_impl->getReqs(desc);
      auto allo = m_heaps.allocate(m_impl.get(), memRes);
      auto data = m_impl->createBuffer(allo, desc);
      auto tracker = m_bufferTracker->makeTracker(newId(), allo.allocation, data);
      return Buffer(data, tracker, desc);
    }
    void DeviceData::createBufferView(ShaderViewDescriptor )
    {

    }

    static size_t roundUpMultiple(size_t value, size_t multiple)
    {
      F_ASSERT(multiple, "multiple needs to be power of 2 was %d", multiple);
      return ((value + multiple - 1) / multiple) * multiple;
    }

    static size_t roundUpMultiple2(size_t numToRound, size_t multiple)
    {
      F_ASSERT(multiple && ((multiple & (multiple - 1)) == 0), "multiple needs to be power of 2 was %d", multiple);
      return (numToRound + multiple - 1) & ~(multiple - 1);
    }

    HeapAllocation HeapManager::allocate(prototypes::DeviceImpl* device, MemoryRequirements requirements)
    {
      GpuHeapAllocation alloc{};
      alloc.alignment = static_cast<int>(requirements.alignment);
      alloc.type = requirements.type;
      auto createHeapBlock = [&](prototypes::DeviceImpl* dev, int index, MemoryRequirements& requirements)
      {
        auto sizeToCreate = roundUpMultiple2(m_minimumHeapSize, requirements.alignment);
        if (requirements.bytes > sizeToCreate)
        {
          sizeToCreate = roundUpMultiple2(requirements.bytes, requirements.alignment);
        }
        std::string name;
        name += std::to_string(index);
        name += "Heap";
        name += std::to_string(static_cast<int>(requirements.type));
        name += "a";
        name += std::to_string(requirements.alignment);
        HeapDescriptor desc = HeapDescriptor()
          .setSizeInBytes(sizeToCreate)
          .setHeapType(requirements.type)
          .setHeapAlignment(requirements.alignment)
          .setName(name);
        F_SLOG("Graphics","Created heap \"%s\" size %zu\n", name.c_str(), sizeToCreate);
        return HeapBlock{ index, PageAllocator(requirements.alignment, sizeToCreate / requirements.alignment), dev->createHeap(desc) };
      };

      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.alignment == requirements.alignment
            && vec.type      == requirements.type;
      });
      if (vectorPtr != m_heaps.end()) // found alignment
      {
        PageBlock block{ -1, -1 };
        for (auto& heap : vectorPtr->heaps)
        {
          if (heap.allocator.freesize() > requirements.bytes) // this will find falsepositives if memory is fragmented.
          {
            block = heap.allocator.allocate(requirements.bytes); // should be cheap anyway
            if (block.valid())
            {
              alloc.block = block;
              alloc.index = heap.index;
              return HeapAllocation{ alloc, heap.heap};
            }
          }
        }
        if (!block.valid())
        {
          // create correct sized heap and allocate from it.
          auto newHeap = createHeapBlock(device, static_cast<int>(vectorPtr->heaps.size()), requirements);
          alloc.block = newHeap.allocator.allocate(requirements.bytes);
          alloc.index = newHeap.index;
          auto heap = newHeap.heap;
          vectorPtr->heaps.emplace_back(std::move(newHeap));
          return HeapAllocation{ alloc, heap };
        }
      }
      HeapVector vec{ static_cast<int>(requirements.alignment), requirements.type, vector<HeapBlock>()};
      auto newHeap = createHeapBlock(device, 0, requirements);
      alloc.block = newHeap.allocator.allocate(requirements.bytes);
      alloc.index = newHeap.index;
      auto heap = newHeap.heap;
      vec.heaps.emplace_back(std::move(newHeap));
      m_heaps.emplace_back(std::move(vec));
      return HeapAllocation{ alloc, heap };
    }

    void HeapManager::release(GpuHeapAllocation object)
    {
      F_ASSERT(object.valid(), "invalid object was released");
      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.alignment == object.alignment
          && vec.type == object.type;
      });
      if (vectorPtr != m_heaps.end())
      {
        vectorPtr->heaps[object.index].allocator.release(object.block);
      }
    }

    vector<GpuHeap> HeapManager::emptyHeaps()
    {
      vector<GpuHeap> emptyHeaps;
      for (auto&& it : m_heaps)
      {
        for (auto&& heap : it.heaps)
        {
          if (heap.allocator.empty())
          {
            emptyHeaps.emplace_back(heap.heap);
          }
        }
      }
      return emptyHeaps;
    }
  }
}

