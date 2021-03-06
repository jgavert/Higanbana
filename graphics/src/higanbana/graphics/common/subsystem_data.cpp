#include "higanbana/graphics/common/subsystem_data.hpp"
#include "higanbana/graphics/common/gpu_group.hpp"
#include "higanbana/graphics/common/implementation.hpp"
#include "higanbana/graphics/common/graphicssurface.hpp"
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/platform/Window.hpp>

namespace higanbana
{
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
        if (implVulkan && (api == GraphicsApi::Vulkan ||api == GraphicsApi::All))
        {
          m_cachedInfos = implVulkan->availableGpus(VendorID::All);
          for (auto&& it : m_cachedInfos) it.api = GraphicsApi::Vulkan;
        }
        if (implDX12 && (api == GraphicsApi::DX12 ||api == GraphicsApi::All))
        {
          vector<GpuInfo> dx12Gpus = implDX12->availableGpus(VendorID::All);
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
        if (info.api == GraphicsApi::All || info.api == api) {
          if (id == VendorID::All || id == info.vendor) {
            info.api = api;
            infos.push_back(info);
          }
        }
      }

      return infos;
    }
    GpuGroup SubsystemData::createGroup(FileSystem& fs, vector<GpuInfo> gpus)
    {
      vector<std::shared_ptr<prototypes::DeviceImpl>> devices;
      vector<GpuInfo> infos;
      for (auto&& info : gpus)
      {
        if (implDX12 && (info.api == GraphicsApi::DX12 || info.api == GraphicsApi::All)) {
          devices.push_back(implDX12->createGpuDevice(fs, info));
          auto cop = info;
          cop.api = GraphicsApi::DX12;
          infos.push_back(cop);
        }
        if (implVulkan && (info.api == GraphicsApi::Vulkan || info.api == GraphicsApi::All)) {
          devices.push_back(implVulkan->createGpuDevice(fs, info));
          auto cop = info;
          cop.api = GraphicsApi::Vulkan;
          infos.push_back(cop);
        }
      }
      return GpuGroup({devices, infos});
    }
    GraphicsSurface SubsystemData::createSurface(Window & window, GpuInfo gpu)
    {
      if (implDX12 && (gpu.api == GraphicsApi::DX12 || gpu.api == GraphicsApi::All)) return implDX12->createSurface(window);
      return implVulkan->createSurface(window);
    }
  }
}