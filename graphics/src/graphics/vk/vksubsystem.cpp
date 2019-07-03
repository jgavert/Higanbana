#include "graphics/vk/vksubsystem.hpp"
#include "graphics/vk/vkdevice.hpp"
#include "graphics/common/gpudevice.hpp"
#include "graphics/common/graphicssurface.hpp"
#include "graphics/vk/vkresources.hpp"
#include "core/Platform/Window.hpp"
#include "core/global_debug.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackNew(
  VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void*                                            /*pUserData*/)
{
  // Supressing unnecessary log messages.

  std::string msgType = "";
  bool breakOn = false;
  if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
  {
    msgType = "GENERAL ";
  }
  else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
  {
    msgType = "PERFORMANCE ";
  }
  else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
  {
    msgType = "VALIDATION ";
  }

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    msgType += "ERROR:";
#if defined(FAZE_PLATFORM_WINDOWS)
    breakOn = true;
#endif
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    msgType += "WARNING:";
#if defined(FAZE_PLATFORM_WINDOWS)
    breakOn = true;
#endif
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    msgType += "Info: ";
  }

  if (!breakOn)
    return false;

  auto pMessage = pCallbackData->pMessage;
  auto messageCode = pCallbackData->messageIdNumber;
  auto bufLabelsCount = pCallbackData->cmdBufLabelCount;
  auto queueLabelsCount = pCallbackData->queueLabelCount;
  auto objectCount = pCallbackData->objectCount;

  F_ILOG("Vulkan/DebugCallback", "{%d}b%dq%do%d: %s", messageCode, bufLabelsCount, queueLabelsCount, objectCount, pMessage);
#if defined(FAZE_PLATFORM_WINDOWS)
  if (breakOn && IsDebuggerPresent())
    __debugbreak();
#endif
  return false;
}

namespace faze
{
  namespace backend
  {
    VulkanSubsystem::VulkanSubsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion, bool debug)
      : m_alloc_info(reinterpret_cast<void*>(&m_allocs), allocs::pfnAllocation, allocs::pfnReallocation, allocs::pfnFree, allocs::pfnInternalAllocation, allocs::pfnInternalFree)
      , m_instance(new vk::Instance, [=](vk::Instance* ist)
        {
          (*ist).destroy(&m_alloc_info);
        })
      , m_debug(debug)
    {
#ifdef FAZE_GRAPHICS_VALIDATION_LAYER
      m_debug = true;
#endif
      /////////////////////////////////
      // getting debug layers
      // TODO: These need to be saved for device creation also. which layers and extensions each use.
      auto layersInfosRes = vk::enumerateInstanceLayerProperties();
      VK_CHECK_RESULT(layersInfosRes);
      auto layersInfos = layersInfosRes.value;

#ifdef FAZE_GRAPHICS_EXTRA_INFO

      GFX_ILOG("Available layers for instance:");
      for (auto&& it : layersInfos)
      {
        GFX_ILOG("\t\t%s", it.layerName);
      }

#endif
      // lunargvalidation list order
      std::vector<std::string> layerOrder = {
      };
      if (m_debug) layerOrder.emplace_back("VK_LAYER_LUNARG_standard_validation");

      std::vector<const char*> layers;
      {
        // lunargvalidation list order
        GFX_ILOG("Enabled Vulkan debug layers:");
        for (auto&& it : layerOrder)
        {
          auto found = std::find_if(layersInfos.begin(), layersInfos.end(), [&](const vk::LayerProperties & layer)
            {
              return it == layer.layerName;
            });
          if (found != layersInfos.end())
          {
            layers.push_back(it.c_str());
            m_layers.push_back(*found);
            GFX_ILOG("\t\t%s", found->layerName);
          }
          else
          {
            GFX_ILOG("not enabled %s", it.c_str());
          }
        }
      }
      /////////////////////////////////
      // Getting extensions
      auto extRes = vk::enumerateInstanceExtensionProperties();
      VK_CHECK_RESULT(extRes);
      std::vector<vk::ExtensionProperties> extInfos = extRes.value;

#ifdef FAZE_GRAPHICS_EXTRA_INFO

      GFX_ILOG("Available extensions for instance:");
      for (auto&& it : extInfos)
      {
        GFX_ILOG("\t\t%s", it.extensionName);
      }

#endif

      std::vector<std::string> extOrder = {
        VK_KHR_SURFACE_EXTENSION_NAME
        , VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        , VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME
        , VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME
        , VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME
        , VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME
        , VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
  #if defined(FAZE_PLATFORM_WINDOWS)
          , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
  #endif
      };
      if (m_debug) extOrder.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

      std::vector<const char*> extensions;
      {
        // lunargvalidation list order
        //GFX_ILOG("Enabled Vulkan extensions:");

        for (auto&& it : extOrder)
        {
          auto found = std::find_if(extInfos.begin(), extInfos.end(), [&](const vk::ExtensionProperties & layer)
            {
              return it == layer.extensionName;
            });
          if (found != extInfos.end())
          {
            extensions.push_back(it.c_str());
            m_extensions.push_back(*found);
            GFX_ILOG("\t\t%s", found->extensionName);
          }
          else
          {
            //GFX_ILOG("not enabled %s", it.c_str());
          }
        }
      }

      /////////////////////////////////
      // app instance
      app_info = vk::ApplicationInfo()
        .setPApplicationName(appName)
        .setApplicationVersion(appVersion)
        .setPEngineName(engineName)
        .setEngineVersion(engineVersion)
        .setApiVersion(VK_API_VERSION_1_1);

      instance_info = vk::InstanceCreateInfo()
        .setPApplicationInfo(&app_info)
        .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
        .setPpEnabledLayerNames(layers.data())
        .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
        .setPpEnabledExtensionNames(extensions.data());

      vk::Result res = vk::createInstance(&instance_info, &m_alloc_info, m_instance.get());

      if (res != vk::Result::eSuccess)
      {
        // quite hot shit baby yeah!
        auto error = vk::to_string(res);
        GFX_ILOG("Instance creation error: %s", error.c_str());
        F_ASSERT(false, "");
      }
      auto devRes = m_instance->enumeratePhysicalDevices();
      VK_CHECK_RESULT(devRes);
      m_devices = devRes.value;

      // get addresses for few functions
      PFN_vkCreateDebugUtilsMessengerEXT dbgCreateDebugUtilsCallback;
      PFN_vkDestroyDebugUtilsMessengerEXT dbgDestroyDebugUtilsCallback;

      dbgCreateDebugUtilsCallback =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          *m_instance, "vkCreateDebugUtilsMessengerEXT");
      if (!dbgCreateDebugUtilsCallback)
      {
        GFX_ILOG("GetInstanceProcAddr: Unable to find vkCreateDebugUtilsMessengerEXT function.");;
      }
      else
      {
        dbgDestroyDebugUtilsCallback =
          (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            *m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (!dbgDestroyDebugUtilsCallback)
        {
          GFX_ILOG("GetInstanceProcAddr: Unable to find vkDestroyDebugUtilsMessengerEXT function.");
        }
        // the debug things
        auto flags = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral;
        auto severityFlags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
        vk::DebugUtilsMessengerCreateInfoEXT info = vk::DebugUtilsMessengerCreateInfoEXT()
          .setMessageSeverity(severityFlags)
          .setMessageType(flags)
          .setPfnUserCallback(debugCallbackNew);

        auto lol = m_instance; // callback needs to keep instance alive until its destroyed... so this works :DD
        auto allocInfo = m_alloc_info;
        m_debugcallback = std::shared_ptr<vk::DebugUtilsMessengerEXT>(new vk::DebugUtilsMessengerEXT, [lol, allocInfo, dbgDestroyDebugUtilsCallback](vk::DebugUtilsMessengerEXT * ist)
          {
            dbgDestroyDebugUtilsCallback(*lol, *ist, reinterpret_cast<const VkAllocationCallbacks*>(&allocInfo));
          });
        dbgCreateDebugUtilsCallback(*m_instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&info), reinterpret_cast<const VkAllocationCallbacks*>(&m_alloc_info), reinterpret_cast<VkDebugUtilsMessengerEXT*>(m_debugcallback.get()));
      }
    }

    std::string VulkanSubsystem::gfxApi()
    {
      return "Vulkan";
    }

    vector<GpuInfo> VulkanSubsystem::availableGpus()
    {
      auto canPresent = [](vk::PhysicalDevice dev)
      {
        auto queueProperties = dev.getQueueFamilyProperties();
        for (auto&& queueProp : queueProperties)
        {
          for (uint32_t i = 0; i < queueProp.queueCount; ++i)
          {
#if defined(FAZE_PLATFORM_WINDOWS)
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
      for (devId = 0; devId < static_cast<int>(m_devices.size()); ++devId)
      {
        GpuInfo info{};
        auto stuff = m_devices[devId].getProperties();
        if (canPresent(m_devices[devId]))
        {
          info.canPresent = true;
        }
        info.id = devId;
        info.name = std::string(stuff.deviceName);
        info.vendor = VendorID::Unknown;
        if (stuff.vendorID == 4098)
        {
          info.vendor = VendorID::Amd;
        }
        else if (stuff.vendorID == 4318)
        {
          info.vendor = VendorID::Nvidia;
        }
        else if (stuff.vendorID == 32902)
        {
          info.vendor = VendorID::Intel;
        }
        switch (stuff.deviceType)
        {
        case vk::PhysicalDeviceType::eIntegratedGpu:
          info.type = DeviceType::IntegratedGpu;
          break;
        case vk::PhysicalDeviceType::eDiscreteGpu:
          info.type = DeviceType::DiscreteGpu;
          break;
        case vk::PhysicalDeviceType::eVirtualGpu:
          info.type = DeviceType::VirtualGpu;
          break;
        case vk::PhysicalDeviceType::eCpu:
          info.type = DeviceType::Cpu;
          break;
        default:
          info.type = DeviceType::Unknown;
          break;
        }


        info.apiVersion = stuff.apiVersion;
        info.apiVersionStr += std::to_string(VK_VERSION_MAJOR(stuff.apiVersion));
        info.apiVersionStr += ".";
        info.apiVersionStr += std::to_string(VK_VERSION_MINOR(stuff.apiVersion));
        info.apiVersionStr += ".";
        info.apiVersionStr += std::to_string(VK_VERSION_PATCH(stuff.apiVersion));

        if (info.type == DeviceType::DiscreteGpu)
        {
          auto memProp = m_devices[devId].getMemoryProperties();
          auto filter = vk::MemoryPropertyFlagBits::eDeviceLocal;
          for (unsigned i = 0; i < memProp.memoryTypeCount; ++i)
          {
            if ((memProp.memoryTypes[i].propertyFlags & filter) == filter)
            {
              info.memory = memProp.memoryHeaps[memProp.memoryTypes[i].heapIndex].size;
              break;
            }
          }
        }

        infos.emplace_back(info);
      }

      return infos;
    }

    std::shared_ptr<backend::prototypes::DeviceImpl> VulkanSubsystem::createGpuDevice(FileSystem& fs, GpuInfo gpu)
    {
      auto&& physDev = m_devices[gpu.id];

      auto physExtRes = physDev.enumerateDeviceExtensionProperties();
      VK_CHECK_RESULT(physExtRes);
      std::vector<vk::ExtensionProperties> devExts = physExtRes.value;

      std::vector<const char*> extensions;
      {
#ifdef FAZE_GRAPHICS_EXTRA_INFO
        GFX_ILOG("Available extensions for device:");
        for (auto&& it : devExts)
        {
          GFX_ILOG("\t\t%s", it.extensionName);
        }
#endif
        // lunargvalidation list order
        //GFX_ILOG("Enabled Vulkan extensions for device:");

        for (auto&& it : devExtOrder)
        {
          auto found = std::find_if(devExts.begin(), devExts.end(), [&](const vk::ExtensionProperties & layer)
            {
              return it == layer.extensionName;
            });
          if (found != devExts.end())
          {
            extensions.push_back(it.c_str());
            m_extensions.push_back(*found);
            //GFX_ILOG("\t\t%s", found->extensionName);
          }
        }
      }

      // queue
      auto queueProperties = physDev.getQueueFamilyProperties();
      std::vector<vk::DeviceQueueCreateInfo> queueInfos;
      uint32_t i = 0;
      std::vector<std::vector<float>> priorities;
      for (auto&& queueProp : queueProperties)
      {
        priorities.push_back(std::vector<float>());
        for (size_t k = 0; k < queueProp.queueCount; ++k)
        {
          priorities[i].push_back(1.f);
        }
        vk::DeviceQueueCreateInfo queueInfo = vk::DeviceQueueCreateInfo()
          .setQueueFamilyIndex(i)
          .setQueueCount(queueProp.queueCount)
          .setPQueuePriorities(priorities[i].data());
        // queue priorities go here.
        queueInfos.push_back(queueInfo);
        ++i;
      }

      auto features = physDev.getFeatures(); // ALL OF THEM FEATURES.

      auto device_info = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
        .setPQueueCreateInfos(queueInfos.data())
        .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
        .setPpEnabledExtensionNames(extensions.data())
        .setPEnabledFeatures(&features);

      auto devRes = physDev.createDevice(device_info);
      VK_CHECK_RESULT(devRes);
      vk::Device dev = devRes.value;

      vk::DispatchLoaderDynamic loader(*m_instance, dev);

      std::shared_ptr<VulkanDevice> impl = std::make_shared<VulkanDevice>(dev, physDev, loader, fs, queueProperties, gpu, false);

      return impl;
    }

#if defined(FAZE_PLATFORM_WINDOWS)
    GraphicsSurface VulkanSubsystem::createSurface(Window& window)
    {
      vk::Win32SurfaceCreateInfoKHR createInfo = vk::Win32SurfaceCreateInfoKHR()
        .setHwnd(window.getInternalWindow().getHWND())
        .setHinstance(window.getInternalWindow().getHInstance());

      auto surfaceKhrRes = m_instance->createWin32SurfaceKHR(createInfo);
      VK_CHECK_RESULT(surfaceKhrRes);
      vk::SurfaceKHR surfacekhr = surfaceKhrRes.value;

      auto inst = m_instance;

      std::shared_ptr<vk::SurfaceKHR> khrSur(new vk::SurfaceKHR(surfacekhr), [inst](vk::SurfaceKHR * ist)
        {
          inst->destroySurfaceKHR(*ist);
          delete ist;
        });

      return GraphicsSurface(std::make_shared<VulkanGraphicsSurface>(khrSur));
    }
#else
    GraphicsSurface VulkanSubsystem::createSurface(Window&)
    {

      std::shared_ptr<vk::SurfaceKHR> khrSur(new vk::SurfaceKHR(), [](vk::SurfaceKHR * ist)
        {
          delete ist;
        });

      return GraphicsSurface(std::make_shared<VulkanGraphicsSurface>(khrSur));
    }
#endif
  }
}
