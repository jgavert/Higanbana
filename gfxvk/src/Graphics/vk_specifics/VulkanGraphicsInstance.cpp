#include "VulkanGraphicsInstance.hpp"

#if defined(PLATFORM_WINDOWS)
#include <intrin.h>
#endif

VulkanGraphicsInstance::VulkanGraphicsInstance()
  : m_alloc_info(reinterpret_cast<void*>(&m_allocs), allocs::pfnAllocation, allocs::pfnReallocation, allocs::pfnFree, allocs::pfnInternalAllocation, allocs::pfnInternalFree)

  , m_instance(new vk::Instance, [=](vk::Instance* ist)
    {
      (*ist).destroy(&m_alloc_info);
    })
{
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugReportFlagsEXT                       flags,
  VkDebugReportObjectTypeEXT                  /*objectType*/,
  uint64_t                                    /*object*/,
  size_t                                      /*Location*/,
  int32_t                                     messageCode,
  const char*                                 pLayerPrefix,
  const char*                                 pMessage,
  void*                                       /*pUserData*/)
{
  std::string msgType = "";
  bool breakOn = false;
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
  {
    msgType = "ERROR:";
    breakOn = true;
  }
  else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
  {
    msgType = "WARNING:";
    breakOn = true;
  }
  else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
  {
    msgType = "PERFORMANCE WARNING:";
  }
  else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
  {
    msgType = "INFO:";
  }
  else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
  {
    msgType = "DEBUG:";
  }
  F_ILOG("Vulkan", "%s %s {%d}: %s", msgType.c_str(), pLayerPrefix, messageCode, pMessage);
#if defined(PLATFORM_WINDOWS)
  if (breakOn && IsDebuggerPresent())
    __debugbreak();
#endif
  return false;
}

bool VulkanGraphicsInstance::createInstance(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
{
  /////////////////////////////////
  // getting debug layers
  // TODO: These need to be saved for device creation also. which layers and extensions each use.
  auto layersInfos = vk::enumerateInstanceLayerProperties();

  std::vector<const char*> layers;
  {
    // lunargvalidation list order
    GFX_ILOG("Enabled Vulkan debug layers:");
    for (auto&& it : layerOrder)
    {
      auto found = std::find_if(layersInfos.begin(), layersInfos.end(), [&](const vk::LayerProperties& layer)
      {
        return it == layer.layerName();
      });
      if (found != layersInfos.end())
      {
        layers.push_back(it.c_str());
        m_layers.push_back(*found);
        GFX_ILOG("%s", found->layerName());
      }
      else
      {
        GFX_ILOG("not enabled %s", it.c_str());
      }
    }
  }
  /////////////////////////////////
  // Getting extensions
  std::vector<vk::ExtensionProperties> extInfos = vk::enumerateInstanceExtensionProperties();

  std::vector<const char*> extensions;
  {
    // lunargvalidation list order
    GFX_ILOG("Enabled Vulkan extensions:");

    for (auto&& it : extOrder)
    {
      auto found = std::find_if(extInfos.begin(), extInfos.end(), [&](const vk::ExtensionProperties& layer)
      {
        return it == layer.extensionName();
      });
      if (found != extInfos.end())
      {
        extensions.push_back(it.c_str());
        m_extensions.push_back(*found);
        GFX_ILOG("%s", found->extensionName());
      }
      else
      {
        GFX_ILOG("not enabled %s", it.c_str());
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
    .apiVersion(VK_API_VERSION_1_0);

  instance_info = vk::InstanceCreateInfo()
    .sType(vk::StructureType::eInstanceCreateInfo)
    .pApplicationInfo(&app_info)
    .enabledLayerCount(static_cast<uint32_t>(layers.size()))
    .ppEnabledLayerNames(layers.data())
    .enabledExtensionCount(static_cast<uint32_t>(extensions.size()))
    .ppEnabledExtensionNames(extensions.data());

  vk::Result res = vk::createInstance(&instance_info, &m_alloc_info, m_instance.get());

  if (res != vk::Result::eSuccess)
  {
    // quite hot shit baby yeah!
    auto error = vk::to_string(res);
    GFX_ILOG("Instance creation error: %s", error.c_str());
    return false;
  }
  m_devices = m_instance->enumeratePhysicalDevices();

  // get addresses for few functions
  PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
  PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;

  dbgCreateDebugReportCallback =
    (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
      *m_instance.get(), "vkCreateDebugReportCallbackEXT");
  if (!dbgCreateDebugReportCallback) {
    GFX_ILOG("GetInstanceProcAddr: Unable to find vkCreateDebugReportCallbackEXT function.");;
    return true;
  }

  dbgDestroyDebugReportCallback =
    (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
      *m_instance.get(), "vkDestroyDebugReportCallbackEXT");
  if (!dbgDestroyDebugReportCallback) {
    GFX_ILOG("GetInstanceProcAddr: Unable to find vkDestroyDebugReportCallbackEXT function.");;
    return true;
  }
  // the debug things
  auto flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eDebug;
  vk::DebugReportCallbackCreateInfoEXT info = vk::DebugReportCallbackCreateInfoEXT(flags, debugCallback, nullptr);

  auto lol = m_instance; // callback needs to keep instance alive until its destroyed... so this works :DD
  auto allocInfo = m_alloc_info;
  m_debugcallback = std::shared_ptr<vk::DebugReportCallbackEXT>(new vk::DebugReportCallbackEXT, [lol, allocInfo, dbgDestroyDebugReportCallback](vk::DebugReportCallbackEXT* ist)
  {
    dbgDestroyDebugReportCallback(*lol.get(), *ist, reinterpret_cast<const VkAllocationCallbacks*>(&allocInfo));
  });
  dbgCreateDebugReportCallback(*m_instance.get(), reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&info), reinterpret_cast<const VkAllocationCallbacks*>(&m_alloc_info), reinterpret_cast<VkDebugReportCallbackEXT*>(m_debugcallback.get()));
  return true;
}

VulkanGpuDevice VulkanGraphicsInstance::createGpuDevice()
{

  auto canPresent = [](vk::PhysicalDevice dev)
  {
    auto queueProperties = dev.getQueueFamilyProperties();
    for (auto&& queueProp : queueProperties)
    {
      for (uint32_t i = 0; i < queueProp.queueCount(); ++i)
      {
#if defined(PLATFORM_WINDOWS)
        if (dev.getWin32PresentationSupportKHR(i))
#endif
        {
          return true;
        }
      }
    }
    return false;
  };
  int devId = 0;
  for (devId = 0; devId < m_devices.size(); ++devId)
  {
    if (canPresent(m_devices[devId]))
      break;
  }
  auto&& physDev = m_devices[devId]; // assuming first device is best
  // layers
  auto devLayers = physDev.enumerateDeviceLayerProperties();
  std::vector<const char*> layers;
  {
    // lunargvalidation list order
    GFX_ILOG("Enabled Vulkan debug layers for device:");
    for (auto&& it : layerOrder)
    {
      auto found = std::find_if(devLayers.begin(), devLayers.end(), [&](const vk::LayerProperties& layer)
      {
        return it == layer.layerName();
      });
      if (found != devLayers.end())
      {
        layers.push_back(it.c_str());
        m_layers.push_back(*found);
        GFX_ILOG("%s", found->layerName());
      }
    }
  }
  // extensions
  std::vector<vk::ExtensionProperties> devExts = physDev.enumerateDeviceExtensionProperties();

  std::vector<const char*> extensions;
  {
    // lunargvalidation list order
    GFX_ILOG("Enabled Vulkan extensions for device:");

    for (auto&& it : devExtOrder)
    {
      auto found = std::find_if(devExts.begin(), devExts.end(), [&](const vk::ExtensionProperties& layer)
      {
        return it == layer.extensionName();
      });
      if (found != devExts.end())
      {
        extensions.push_back(it.c_str());
        m_extensions.push_back(*found);
        GFX_ILOG("found %s", found->extensionName());
      }
    }
  }
  // queue


  //auto queueProperties = vk::getPhysicalDeviceQueueFamilyProperties(physDev);
  auto queueProperties = physDev.getQueueFamilyProperties();
  std::vector<vk::DeviceQueueCreateInfo> queueInfos;
  uint32_t i = 0;
  std::vector<std::vector<float>> priorities;
  for (auto&& queueProp : queueProperties)
  {
    priorities.push_back(std::vector<float>());
    for (size_t k = 0; k < queueProp.queueCount(); ++k)
    {
      priorities[i].push_back(1.f);
    }
    vk::DeviceQueueCreateInfo queueInfo = vk::DeviceQueueCreateInfo()
      .sType(vk::StructureType::eDeviceQueueCreateInfo)
      .queueFamilyIndex(i)
      .queueCount(queueProp.queueCount())
      .pQueuePriorities(priorities[i].data());
    // queue priorities go here.
    queueInfos.push_back(queueInfo);
    ++i;
  }

  auto features = physDev.getFeatures(); // ALL OF THEM FEATURES.

  auto heapInfos = physDev.getMemoryProperties();

  auto device_info = vk::DeviceCreateInfo()
    .sType(vk::StructureType::eDeviceCreateInfo)
    .queueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
    .pQueueCreateInfos(queueInfos.data())
    .enabledLayerCount(static_cast<uint32_t>(layers.size()))
    .ppEnabledLayerNames(layers.data())
    .enabledExtensionCount(static_cast<uint32_t>(extensions.size()))
    .ppEnabledExtensionNames(extensions.data())
    .pEnabledFeatures(&features);


  vk::Device dev = physDev.createDevice(device_info, m_alloc_info);
  std::shared_ptr<vk::Device> device(new vk::Device(dev), [=](vk::Device* ist)
  {
    ist->destroy(&m_alloc_info);
  });

  return VulkanGpuDevice(device, m_alloc_info, queueProperties, heapInfos, false);
}
