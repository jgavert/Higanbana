#include "faze/src/new_gfx/common/gpudevice.hpp"
#include "faze/src/new_gfx/common/graphicssurface.hpp"
#include "vkresources.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/global_debug.hpp"


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackNew(
  VkDebugReportFlagsEXT                       flags,
  VkDebugReportObjectTypeEXT                  /*objectType*/,
  uint64_t                                    /*object*/,
  size_t                                      /*Location*/,
  int32_t                                     messageCode,
  const char*                                 pLayerPrefix,
  const char*                                 pMessage,
  void*                                       /*pUserData*/)
{
  // Supressing unnecessary log messages.

  std::string msgType = "";
#if defined(FAZE_PLATFORM_WINDOWS)
  bool breakOn = false;
#endif
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
  {
    msgType = "ERROR:";
#if defined(FAZE_PLATFORM_WINDOWS)
    breakOn = true;
#endif
  }
  else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
  {
    msgType = "WARNING:";
#if defined(FAZE_PLATFORM_WINDOWS)
    breakOn = true;
#endif
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
  if (!breakOn && ( std::string(pLayerPrefix) == "loader" || std::string(pLayerPrefix) == "DebugReport"))
    return false;
  F_ILOG("Vulkan/DebugCallback", "%s %s {%d}: %s", msgType.c_str(), pLayerPrefix, messageCode, pMessage);
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
    VulkanSubsystem::VulkanSubsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion)
      : m_alloc_info(reinterpret_cast<void*>(&m_allocs), allocs::pfnAllocation, allocs::pfnReallocation, allocs::pfnFree, allocs::pfnInternalAllocation, allocs::pfnInternalFree)
      , m_instance(new vk::Instance, [=](vk::Instance* ist)
    {
      (*ist).destroy(&m_alloc_info);
    })
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
      std::vector<vk::ExtensionProperties> extInfos = vk::enumerateInstanceExtensionProperties();

      std::vector<const char*> extensions;
      {
        // lunargvalidation list order
        GFX_ILOG("Enabled Vulkan extensions:");

        for (auto&& it : extOrder)
        {
          auto found = std::find_if(extInfos.begin(), extInfos.end(), [&](const vk::ExtensionProperties& layer)
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
            GFX_ILOG("not enabled %s", it.c_str());
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
        .setApiVersion(VK_MAKE_VERSION(1, 0, 42));

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
      m_devices = m_instance->enumeratePhysicalDevices();

      // get addresses for few functions
      PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
      PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;

      dbgCreateDebugReportCallback =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
          *m_instance, "vkCreateDebugReportCallbackEXT");
      if (!dbgCreateDebugReportCallback)
      {
        GFX_ILOG("GetInstanceProcAddr: Unable to find vkCreateDebugReportCallbackEXT function.");;
      }

      dbgDestroyDebugReportCallback =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
          *m_instance, "vkDestroyDebugReportCallbackEXT");
      if (!dbgDestroyDebugReportCallback)
      {
        GFX_ILOG("GetInstanceProcAddr: Unable to find vkDestroyDebugReportCallbackEXT function.");
      }
      // the debug things
      auto flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::ePerformanceWarning;
      vk::DebugReportCallbackCreateInfoEXT info = vk::DebugReportCallbackCreateInfoEXT(flags, debugCallbackNew, nullptr);

      auto lol = m_instance; // callback needs to keep instance alive until its destroyed... so this works :DD
      auto allocInfo = m_alloc_info;
      m_debugcallback = std::shared_ptr<vk::DebugReportCallbackEXT>(new vk::DebugReportCallbackEXT, [lol, allocInfo, dbgDestroyDebugReportCallback](vk::DebugReportCallbackEXT* ist)
      {
        dbgDestroyDebugReportCallback(*lol, *ist, reinterpret_cast<const VkAllocationCallbacks*>(&allocInfo));
      });
      dbgCreateDebugReportCallback(*m_instance, reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&info), reinterpret_cast<const VkAllocationCallbacks*>(&m_alloc_info), reinterpret_cast<VkDebugReportCallbackEXT*>(m_debugcallback.get()));
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

    GpuDevice VulkanSubsystem::createGpuDevice(FileSystem& fs, GpuInfo gpu)
    {
      auto&& physDev = m_devices[gpu.id];

      std::vector<vk::ExtensionProperties> devExts = physDev.enumerateDeviceExtensionProperties();

      std::vector<const char*> extensions;
      {
        GFX_ILOG("Available extensions for device:");
        for (auto&& it : devExts)
        {
          GFX_ILOG("\t\t%s", it.extensionName);
        }
        // lunargvalidation list order
        GFX_ILOG("Enabled Vulkan extensions for device:");

        for (auto&& it : devExtOrder)
        {
          auto found = std::find_if(devExts.begin(), devExts.end(), [&](const vk::ExtensionProperties& layer)
          {
            return it == layer.extensionName;
          });
          if (found != devExts.end())
          {
            extensions.push_back(it.c_str());
            m_extensions.push_back(*found);
            GFX_ILOG("\t\t%s", found->extensionName);
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

      vk::Device dev = physDev.createDevice(device_info);

      std::shared_ptr<VulkanDevice> impl = std::make_shared<VulkanDevice>(dev, physDev, fs, queueProperties, gpu, false);

      return GpuDevice(DeviceData(impl));
    }

    GraphicsSurface VulkanSubsystem::createSurface(Window& window)
    {
      vk::Win32SurfaceCreateInfoKHR createInfo = vk::Win32SurfaceCreateInfoKHR()
        .setHwnd(window.getInternalWindow().getHWND())
        .setHinstance(window.getInternalWindow().getHInstance());


      vk::SurfaceKHR surfacekhr = m_instance->createWin32SurfaceKHR(createInfo);

      auto inst = m_instance;

      std::shared_ptr<vk::SurfaceKHR> khrSur(new vk::SurfaceKHR(surfacekhr), [inst](vk::SurfaceKHR* ist)
      {
        inst->destroySurfaceKHR(*ist);
        delete ist;
      });

      return GraphicsSurface(std::make_shared<VulkanGraphicsSurface>(khrSur));
    }
  }
}