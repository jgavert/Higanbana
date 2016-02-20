#pragma once

#include "GpuDevice.hpp"
#include "vk_specifics/AllocStuff.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include "core/src/global_debug.hpp"
#include "gfxDebug.hpp"
#include <utility>
#include <string>

#include <vulkan/vk_cpp.h>

class GpuInfo
{
public:
  GpuInfo()
    : vendorId(0)
    , deviceId(0)
    , subSysId(0)
    , revision(0)
    , dedVidMem(0)
    , shaSysMem(0)
  {}
  std::string description;
  unsigned vendorId;
  unsigned deviceId;
  unsigned subSysId;
  unsigned revision;
  size_t dedVidMem;
  size_t dedSysMem;
  size_t shaSysMem;
};

class GraphicsInstance
{
private:
  std::vector<GpuInfo> infos;
  allocs m_allocs;
  vk::AllocationCallbacks m_alloc_info;
  vk::ApplicationInfo app_info;
  vk::InstanceCreateInfo instance_info;
  FazPtr<vk::Instance> instance;
  // 2 device support only
  int betterDevice;
  int lesserDevice;

public:
  GraphicsInstance()
    : m_alloc_info(reinterpret_cast<void*>(&m_allocs), allocs::pfnAllocation, allocs::pfnReallocation, allocs::pfnFree, allocs::pfnInternalAllocation, allocs::pfnInternalFree)

    , instance([=](vk::Instance ist)
      {
        vk::destroyInstance(ist, &m_alloc_info);
      })
    ,betterDevice(-1), lesserDevice(-1)
  {
  }

  bool createInstance(const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1)
  {
    /////////////////////////////////
    // getting debug layers
    // TODO: These need to be saved for device creation also. which layers and extensions each use.
    std::vector<vk::LayerProperties> layersInfos;
    vk::enumerateInstanceLayerProperties(layersInfos);

    std::vector<const char*> layers;
    {
      // lunargvalidation list order
      std::vector<std::string> layerOrder = {
#if defined(DEBUG)
        "VK_LAYER_LUNARG_threading"
        ,"VK_LAYER_LUNARG_param_checker"
        ,"VK_LAYER_LUNARG_device_limits"
        ,"VK_LAYER_LUNARG_object_tracker"
        ,"VK_LAYER_LUNARG_image"
        ,"VK_LAYER_LUNARG_mem_tracker"
        ,"VK_LAYER_LUNARG_draw_state"
#endif
        ,"VK_LAYER_LUNARG_swapchain"
#if defined(DEBUG)
        ,"VK_LAYER_GOOGLE_unique_objects"
#endif
      };
      F_LOG("Enabled Vulkan debug layers:\n");
      for (auto&& it : layerOrder)
      {
        auto found = std::find_if(layersInfos.begin(), layersInfos.end(), [&](const vk::LayerProperties& layer)
        {
          return it == layer.layerName();
        });
        if (found != layersInfos.end())
        {
          layers.push_back(found->layerName());
          F_LOG("%s\n", found->layerName());
        }
      }
    }

    /////////////////////////////////
    // Getting extensions
    std::vector<vk::ExtensionProperties> extInfos;
    vk::enumerateInstanceExtensionProperties("", extInfos);

    std::vector<const char*> extensions;
    {
      // lunargvalidation list order
      //VkSurfaceKHR
      std::vector<std::string> layerOrder = {
        VK_KHR_SURFACE_EXTENSION_NAME
#if defined(PF_WINDOWS)
      , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
#if defined(DEBUG)
      , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
      };
      F_LOG("Enabled Vulkan extensions:\n");

      for (auto&& it : layerOrder)
      {
        auto found = std::find_if(extInfos.begin(), extInfos.end(), [&](const vk::ExtensionProperties& layer)
        {
          return it == layer.extensionName();
        });
        if (found != extInfos.end())
        {
          extensions.push_back(found->extensionName());
          F_LOG("found %s\n", found->extensionName());
        }
      }
    }

    /////////////////////////////////
    // app instance
    app_info = vk::ApplicationInfo()
      .sType(vk::StructureType::eApplicationInfo)
      .pApplicationName(appName)
      .applicationVersion(appVersion)
      .pEngineName(engineName)
      .engineVersion(engineVersion)
      .apiVersion(VK_API_VERSION);

    instance_info = vk::InstanceCreateInfo()
      .sType(vk::StructureType::eInstanceCreateInfo)
      .pApplicationInfo(&app_info)
      .enabledLayerCount(static_cast<uint32_t>(layers.size()))
      .ppEnabledLayerNames(layers.data())
      .enabledExtensionCount(static_cast<uint32_t>(extensions.size()))
      .ppEnabledExtensionNames(extensions.data());


    vk::Result res = vk::createInstance(&instance_info, &m_alloc_info, instance.get());

    if (res != vk::Result::eVkSuccess)
    {
      // quite hot shit baby yeah!
      F_LOG("Instance creation error: %s\n", vk::getString(res));
      return false;
    }

    // since creation was success -> lets enumerate the physical devices

    return true;
  }

  GpuDevice CreateGpuDevice(bool debug = true, bool warpDriver = true)
  {
#ifdef DEBUG
    debug = true;
	  warpDriver = false;
#endif
    return CreateGpuDevice(betterDevice, debug, warpDriver);
  }

  GpuDevice CreateGpuDevice(int , bool debug /*= false*/, bool  /*warpDevice = true*/)
  {
    return GpuDevice(nullptr, debug);
  }

  GpuInfo getInfo(int num)
  {
    if (num < 0 || num >= infos.size())
      return{};
    return infos[num];
  }

  int DeviceCount()
  {
    return static_cast<int>(infos.size());
  }

};
