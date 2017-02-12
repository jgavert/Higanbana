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

    GpuHeapAllocation HeapManager::allocate(MemoryRequirements )
    {
     /*
      auto vectorPtr = std::find_if(m_heaps.begin(), m_heaps.end(), [&](HeapVector& vec)
      {
        return vec.alignment == requirements.alignment;
      });
      if (vectorPtr != m_heaps.end())
      {
        auto freeHeap = std::find_if(vectorPtr->heaps.begin(), vectorPtr->heaps.end(), [&](HeapBlock& vec)
        {
          return vec.allocator.freeContinuosSize() > requirements.bytes;
        });
        if (freeHeap != vectorPtr->heaps.end())
        {
          auto block = freeHeap->allocator.allocate(requirements.bytes);
          GpuHeapAllocation alloc{};
          alloc.block = block;
          alloc.alignment = requirements.alignment;
          alloc.index = freeHeap->index;
          return alloc;
        }
        else
        {
          
        }
      }
      */
      GpuHeapAllocation alloc{};
      return alloc;
    }

    void HeapManager::release(GpuHeapAllocation )
    {

    }
  }
}

