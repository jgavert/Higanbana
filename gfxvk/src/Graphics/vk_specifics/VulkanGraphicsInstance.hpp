#pragma once

#include "VulkanGpuDevice.hpp"
#include "AllocStuff.hpp"
#include "core/src/memory/ManagedResource.hpp"
#include "core/src/global_debug.hpp"
#include "gfxvk/src/Graphics/GpuInfo.hpp"

#include <utility>
#include <string>

#include <vulkan/vk_cpp.h>


class VulkanGraphicsInstance
{
private:
  std::vector<GpuInfo> infos;
  allocs m_allocs;
  vk::AllocationCallbacks m_alloc_info;
  vk::ApplicationInfo app_info;
  vk::InstanceCreateInfo instance_info;
  std::vector<vk::ExtensionProperties> m_extensions;
  std::vector<vk::LayerProperties> m_layers;
  std::vector<vk::PhysicalDevice> m_devices;
  FazPtrVk<vk::Instance> m_instance;
  FazPtrVk<vk::DebugReportCallbackEXT> m_debugcallback;

  // lunargvalidation list order
  std::vector<std::string> layerOrder = {
#if defined(DEBUG)
    "VK_LAYER_LUNARG_threading",
    "VK_LAYER_LUNARG_param_checker",
    "VK_LAYER_LUNARG_device_limits",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_image",
    "VK_LAYER_LUNARG_mem_tracker",
    "VK_LAYER_LUNARG_draw_state",
#endif
    "VK_LAYER_LUNARG_swapchain"
#if defined(DEBUG)
    // disable for device creation tests
    ,"VK_LAYER_GOOGLE_unique_objects" 
    // apparently was created because the lunarg layers expect that all objects will be unique - which isn't guaranteed for non-dispatchable objects
#endif
  };

  std::vector<std::string> extOrder = {
    VK_KHR_SURFACE_EXTENSION_NAME
#if defined(PF_WINDOWS)
    , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
#if defined(DEBUG)
    , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
  };

  std::vector<std::string> devExtOrder = {
    "VK_KHR_swapchain"
  };

public:
  VulkanGraphicsInstance();
  bool createInstance(const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1);
  VulkanGpuDevice createGpuDevice();
};

using GraphicsInstanceImpl = VulkanGraphicsInstance;