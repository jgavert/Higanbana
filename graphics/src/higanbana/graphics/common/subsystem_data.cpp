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

  namespace backend
  {
    SubsystemData::SubsystemData(const char* appName, bool debugLayer, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : implDX12(nullptr)
      , implVulkan(nullptr)
      , appName(appName)
      , appVersion(appVersion)
      , engineName(engineName)
      , engineVersion(engineVersion)
    {
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#if defined(HIGANBANA_GRAPHICS_AD_DX12)
      implDX12 = std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion, debugLayer);
#endif
#endif
#if defined(HIGANBANA_GRAPHICS_AD_VULKAN)
      implVulkan = std::make_shared<VulkanSubsystem>(appName, appVersion, engineName, engineVersion, debugLayer);
#endif
    }
    vector<GpuInfo> SubsystemData::availableGpus(GraphicsApi api)
    {
      vector<GpuInfo> infos;
      if (implVulkan && api == GraphicsApi::All || api == GraphicsApi::Vulkan)
      {
        infos = implVulkan->availableGpus();
        for (auto&& it : infos) it.api = GraphicsApi::Vulkan;
      }
      if (implDX12 && (api == GraphicsApi::All || api == GraphicsApi::DX12))
      {
        vector<GpuInfo> dx12Gpus = implDX12->availableGpus();
        for (auto&& it : dx12Gpus) it.api = GraphicsApi::DX12;
        infos.insert(infos.end(), dx12Gpus.begin(), dx12Gpus.end());
      }
      return infos;
    }
    GpuGroup SubsystemData::createGroup(FileSystem& fs, vector<GpuInfo> gpus)
    {
      vector<std::shared_ptr<prototypes::DeviceImpl>> devices;
      for (auto&& info : gpus)
      {
        if (info.api == GraphicsApi::DX12)
          devices.push_back(implDX12->createGpuDevice(fs, info));
        else
          devices.push_back(implVulkan->createGpuDevice(fs, info));
      }
      return GpuGroup({devices, gpus});
    }
    GraphicsSurface SubsystemData::createSurface(Window & window, GpuInfo gpu)
    {
      if (gpu.api == GraphicsApi::DX12) return implDX12->createSurface(window);
      return implVulkan->createSurface(window);
    }
  }
}