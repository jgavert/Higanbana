#include "higanbana/graphics/common/subsystem_data.hpp"
#include "higanbana/graphics/common/gpu_group.hpp"
#include "higanbana/graphics/common/implementation.hpp"
#include "higanbana/graphics/common/graphicssurface.hpp"
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/platform/Window.hpp>

namespace higanbana
{
  const char* toString(GraphicsApi api)
  {
    if (api == GraphicsApi::DX12)
      return "DX12";
    return "Vulkan";
  }

  const char* toString(VendorID id)
  {
    if (id == VendorID::Amd)
      return "AMD";
    if (id == VendorID::Nvidia)
      return "Nvidia";
    if (id == VendorID::Intel)
      return "Intel";
    return "Any";
  }

  const char* toString(QueueType id)
  {
    if (id == QueueType::Graphics)
      return "Graphics";
    if (id == QueueType::Compute)
      return "Compute";
    if (id == QueueType::Dma)
      return "Dma";
    if (id == QueueType::External)
      return "External";
    return "Unknown";
  }

  namespace backend
  {
    SubsystemData::SubsystemData(GraphicsApi allowedApi, const char* appName, bool debugLayer, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : implDX12(nullptr)
      , implVulkan(nullptr)
      , appName(appName)
      , appVersion(appVersion)
      , engineName(engineName)
      , engineVersion(engineVersion)
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
      if (allowedApi == GraphicsApi::DX12 || allowedApi == GraphicsApi::All)
      {
        implDX12 = std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion, debugLayer);
      }
#endif
      if (allowedApi == GraphicsApi::Vulkan || allowedApi == GraphicsApi::All)
      {
        implVulkan = std::make_shared<VulkanSubsystem>(appName, appVersion, engineName, engineVersion, debugLayer);
      }
    }
    vector<GpuInfo> SubsystemData::availableGpus(GraphicsApi api, VendorID id, QueryDevicesMode mode)
    {
      if (m_cachedInfos.empty() || mode == QueryDevicesMode::SlowQuery) {
        m_cachedInfos.clear();
        if (implVulkan)
        {
          m_cachedInfos = implVulkan->availableGpus(VendorID::All); // I might have killed RGP with this
          for (auto&& it : m_cachedInfos) it.api = GraphicsApi::Vulkan;
        }
        if (implDX12)
        {
          vector<GpuInfo> dx12Gpus = implDX12->availableGpus(VendorID::All); // TODO: test rgp
          for (auto&& it : dx12Gpus) {
            it.api = GraphicsApi::DX12;
            bool merged = false;
            for (auto&& desc : m_cachedInfos) {
              if (desc.deviceId == it.deviceId) {
                // same device!
                desc = it;
                desc.api = GraphicsApi::All;
                merged = true;
                break;
              }
            }
            if (!merged)
              m_cachedInfos.push_back(it);
          }
        }
      }

      // craft filtered version of all devices here as per request
      vector<GpuInfo> infos;
      for (auto&& info : m_cachedInfos) {
        if (api == GraphicsApi::All || info.api == api) {
          if (id == VendorID::All || id == info.vendor) {
            infos.push_back(info);
          }
        }
      }

      return infos;
    }
    GpuGroup SubsystemData::createGroup(FileSystem& fs, vector<GpuInfo> gpus)
    {
      vector<std::shared_ptr<prototypes::DeviceImpl>> devices;
      for (auto&& info : gpus)
      {
        if (implDX12 && (info.api == GraphicsApi::DX12 || info.api == GraphicsApi::All))
          devices.push_back(implDX12->createGpuDevice(fs, info));
        if (implVulkan && (info.api == GraphicsApi::Vulkan || info.api == GraphicsApi::All))
          devices.push_back(implVulkan->createGpuDevice(fs, info));
      }
      return GpuGroup({devices, gpus});
    }
    GraphicsSurface SubsystemData::createSurface(Window & window, GpuInfo gpu)
    {
      if (implDX12 && (gpu.api == GraphicsApi::DX12 || gpu.api == GraphicsApi::All)) return implDX12->createSurface(window);
      return implVulkan->createSurface(window);
    }
  }
}