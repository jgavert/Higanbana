#include <graphics/common/subsystem_data.hpp>
#include <graphics/common/gpudevice.hpp>
#include <graphics/common/implementation.hpp>
#include <graphics/common/graphicssurface.hpp>

namespace faze
{
  const char* toString(GraphicsApi api)
  {
    if (api == GraphicsApi::DX12)
      return "DX12";
    return "Vulkan";
  }

  namespace backend
  {
    SubsystemData::SubsystemData(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : implDX12(nullptr)
      , implVulkan(nullptr)
      , appName(appName)
      , appVersion(appVersion)
      , engineName(engineName)
      , engineVersion(engineVersion)
    {
#if defined(FAZE_PLATFORM_WINDOWS)
      implDX12 = std::make_shared<DX12Subsystem>(appName, appVersion, engineName, engineVersion);
#endif
      implVulkan = std::make_shared<VulkanSubsystem>(appName, appVersion, engineName, engineVersion);
    }
    vector<GpuInfo> SubsystemData::availableGpus(GraphicsApi api)
    {
      vector<GpuInfo> infos;
      if (api == GraphicsApi::All || api == GraphicsApi::Vulkan)
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
    GpuDevice SubsystemData::createDevice(FileSystem & fs, GpuInfo gpu)
    {
      if (gpu.api == GraphicsApi::DX12) return implDX12->createGpuDevice(fs, gpu);
      return implVulkan->createGpuDevice(fs, gpu);
    }
    GraphicsSurface SubsystemData::createSurface(Window & window, GpuInfo gpu)
    {
      if (gpu.api == GraphicsApi::DX12) return implDX12->createSurface(window);
      return implVulkan->createSurface(window);
    }
  }
}