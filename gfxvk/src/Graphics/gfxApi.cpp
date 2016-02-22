#include "GfxApi.hpp"

#include <intrin.h>

GraphicsInstance::GraphicsInstance()
  : m_alloc_info(reinterpret_cast<void*>(&m_allocs), allocs::pfnAllocation, allocs::pfnReallocation, allocs::pfnFree, allocs::pfnInternalAllocation, allocs::pfnInternalFree)

  , m_instance([=](vk::Instance ist)
    {
      vk::destroyInstance(ist, &m_alloc_info);
    })
  ,betterDevice(-1), lesserDevice(-1)
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
  std::string msgType = "[Vulkan] ";
  bool breakOn = false;
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    msgType + "ERROR: ";
	breakOn = true;
  }
  else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    msgType + "WARNING: ";
	breakOn = true;
  }
  else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
    msgType + "PERFORMANCE WARNING: ";
  }
  else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
    msgType + "INFO: ";
  }
  else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
    msgType + "DEBUG: ";
  }
  F_LOG("%s[%s] Code %d : %s\n", msgType.c_str(), pLayerPrefix, messageCode, pMessage);
  if (breakOn)
	__debugbreak();
  return false;
}

bool GraphicsInstance::createInstance(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
{
  /////////////////////////////////
  // getting debug layers
  // TODO: These need to be saved for device creation also. which layers and extensions each use.
  std::vector<vk::LayerProperties> layersInfos;
  vk::enumerateInstanceLayerProperties(layersInfos);

  std::vector<const char*> layers;
  {
    // lunargvalidation list order
    //F_LOG("Enabled Vulkan debug layers:\n");
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
        //F_LOG("%s\n", found->layerName());
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
    //F_LOG("Enabled Vulkan extensions:\n");

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
        //F_LOG("found %s\n", found->extensionName());
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

  vk::Result res = vk::createInstance(&instance_info, &m_alloc_info, m_instance.get());

  if (res != vk::Result::eVkSuccess)
  {
    // quite hot shit baby yeah!
    F_LOG("Instance creation error: %s\n", vk::getString(res));
    return false;
  }
  vk::enumeratePhysicalDevices(*m_instance.get(), m_devices);

  // get addresses for few functions
  PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
  PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;

  dbgCreateDebugReportCallback =
    (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
      *m_instance.get(), "vkCreateDebugReportCallbackEXT");
  if (!dbgCreateDebugReportCallback) {
    F_LOG("GetInstanceProcAddr: Unable to find vkCreateDebugReportCallbackEXT function.");;
    return true;
  }

  dbgDestroyDebugReportCallback =
    (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
      *m_instance.get(), "vkDestroyDebugReportCallbackEXT");
  if (!dbgDestroyDebugReportCallback) {
    F_LOG("GetInstanceProcAddr: Unable to find vkDestroyDebugReportCallbackEXT function.\n");;
    return true;
  }
  // the debug things
  auto flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eDebug;
  vk::DebugReportCallbackCreateInfoEXT info = vk::DebugReportCallbackCreateInfoEXT(flags, debugCallback, nullptr);

  auto lol = m_instance; // callback needs to keep instance alive until its destroyed... so this works :DD
  auto allocInfo = m_alloc_info;
  m_debugcallback = FazPtr<vk::DebugReportCallbackEXT>([lol, allocInfo, dbgDestroyDebugReportCallback](vk::DebugReportCallbackEXT ist)
  {
    dbgDestroyDebugReportCallback(*lol.get(), ist, reinterpret_cast<const VkAllocationCallbacks*>(&allocInfo));
  });
  dbgCreateDebugReportCallback(*m_instance.get(), reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&info), reinterpret_cast<const VkAllocationCallbacks*>(&m_alloc_info), reinterpret_cast<VkDebugReportCallbackEXT*>(m_debugcallback.get()));
  return true;
}

GpuDevice GraphicsInstance::CreateGpuDevice()
{
  return CreateGpuDevice(0, false, false);
}

GpuDevice GraphicsInstance::CreateGpuDevice(bool, bool)
{
  return CreateGpuDevice(0, false, false);
}

GpuDevice GraphicsInstance::CreateGpuDevice(int)
{
  return CreateGpuDevice(0, false, false);
}

GpuDevice GraphicsInstance::CreateGpuDevice(int , bool, bool)
{

  auto canPresent = [](vk::PhysicalDevice dev)
  {
    auto queueProperties = vk::getPhysicalDeviceQueueFamilyProperties(dev);
    for (auto&& queueProp : queueProperties)
    {
      for (uint32_t i = 0; i < queueProp.queueCount(); ++i)
      {
        if (vk::getPhysicalDeviceWin32PresentationSupportKHR(dev, i))
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
  std::vector<vk::LayerProperties> devLayers;
  vk::enumerateDeviceLayerProperties(physDev, devLayers);
  std::vector<const char*> layers;
  {
    // lunargvalidation list order
    //F_LOG("Enabled Vulkan debug layers for device:\n");
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
        //F_LOG("%s\n", found->layerName());
      }
    }
  }
  // extensions
  std::vector<vk::ExtensionProperties> devExts;
  vk::enumerateDeviceExtensionProperties(physDev,"", devExts);
  std::vector<const char*> extensions;
  {
    // lunargvalidation list order
    //F_LOG("Enabled Vulkan extensions for device:\n");

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
        //F_LOG("found %s\n", found->extensionName());
      }
    }
  }
  // queue


  auto queueProperties = vk::getPhysicalDeviceQueueFamilyProperties(physDev);
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

  vk::PhysicalDeviceFeatures features;
  vk::getPhysicalDeviceFeatures(physDev, features);

  vk::DeviceCreateInfo device_info = vk::DeviceCreateInfo()
    .sType(vk::StructureType::eDeviceCreateInfo)
    .queueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
    .pQueueCreateInfos(queueInfos.data())
    .enabledLayerCount(static_cast<uint32_t>(layers.size()))
    .ppEnabledLayerNames(layers.data())
    .enabledExtensionCount(static_cast<uint32_t>(extensions.size()))
    .ppEnabledExtensionNames(extensions.data())
    .pEnabledFeatures(&features);
   

  FazPtr<vk::Device> device([=](vk::Device ist)
  {
    vk::destroyDevice(ist, &m_alloc_info);
  });

  vk::Result res = vk::createDevice(physDev, device_info, m_alloc_info, device.getRef());
  if (res != vk::Result::eVkSuccess)
  {
    F_LOG("Device creation failed: %s\n", vk::getString(res));
  }
  return GpuDevice(device, m_alloc_info, false);
}

GpuInfo GraphicsInstance::getInfo(int num)
{
  if (num < 0 || num >= infos.size())
    return{};
  return infos[num];
}

int GraphicsInstance::DeviceCount()
{
  return static_cast<int>(infos.size());
}
