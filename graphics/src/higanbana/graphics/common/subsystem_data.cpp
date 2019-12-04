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
      }
        implVulkan = std::make_shared<VulkanSubsystem>(appName, appVersion, engineName, engineVersion, debugLayer);
    }
    vector<GpuInfo> SubsystemData::availableGpus(GraphicsApi api, VendorID id)
    {
      vector<GpuInfo> infos;
      HIGAN_ASSERT(api == GraphicsApi::All || (implVulkan && api == GraphicsApi::Vulkan), "Subsystem wasn't created with Vulkan enabled! check --rgp switch!");
      if (implVulkan && (api == GraphicsApi::All || api == GraphicsApi::Vulkan))
      {
        infos = implVulkan->availableGpus(id);
        for (auto&& it : infos) it.api = GraphicsApi::Vulkan;
      }
      HIGAN_ASSERT(api == GraphicsApi::All || (implDX12 && api == GraphicsApi::DX12), "Subsystem wasn't created with DX12 enabled! check --rgp switch!");
      if (implDX12 && (api == GraphicsApi::All || api == GraphicsApi::DX12))
      {
        vector<GpuInfo> dx12Gpus = implDX12->availableGpus(id);
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