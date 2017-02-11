#pragma once

#include "gfxvk/src/new_gfx/common/resources.hpp"

#include "gfxvk/src/new_gfx/vk/util/AllocStuff.hpp" // refactor
#include <vulkan/vulkan.hpp>

namespace faze
{
  namespace backend
  {
    struct SubsystemImpl
    {
      vector<GpuInfo> infos;

      allocs m_allocs;
      vk::AllocationCallbacks m_alloc_info;
      vk::ApplicationInfo app_info;
      vk::InstanceCreateInfo instance_info;
      vector<vk::ExtensionProperties> m_extensions;
      vector<vk::LayerProperties> m_layers;
      vector<vk::PhysicalDevice> m_devices;
      std::shared_ptr<vk::Instance> m_instance;
      std::shared_ptr<vk::DebugReportCallbackEXT> m_debugcallback;

      // lunargvalidation list order
      std::vector<std::string> layerOrder = {
#if defined(DEBUG)
        "VK_LAYER_LUNARG_standard_validation",
#endif
        "VK_LAYER_LUNARG_swapchain"
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

      SubsystemImpl(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
    };
  }
}