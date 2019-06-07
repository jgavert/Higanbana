#pragma once
#include "vkresources.hpp"

namespace faze
{
  namespace backend
  {

    class VulkanSubsystem : public prototypes::SubsystemImpl
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
      std::shared_ptr<vk::DebugUtilsMessengerEXT> m_debugcallback;

      // lunargvalidation list order
      std::vector<std::string> layerOrder = {
  #if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
          "VK_LAYER_LUNARG_standard_validation",
  #endif
      };

      std::vector<std::string> extOrder = {
        VK_KHR_SURFACE_EXTENSION_NAME
        , VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
  #if defined(FAZE_PLATFORM_WINDOWS)
          , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
  #endif
  #if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
          //, VK_EXT_DEBUG_REPORT_EXTENSION_NAME // VK_EXT_debug_utils
		  , VK_EXT_DEBUG_UTILS_EXTENSION_NAME
  #endif
      };

      std::vector<std::string> devExtOrder = {
        "VK_EXT_shader_subgroup_ballot",
        "VK_EXT_shader_subgroup_vote",
        "VK_KHR_maintenance2",
        "VK_KHR_swapchain",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_get_memory_requirements2"
      };
    public:
      VulkanSubsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
      std::shared_ptr<backend::prototypes::DeviceImpl> createGpuDevice(FileSystem& fs, GpuInfo gpu);
      GraphicsSurface createSurface(Window& window) override;
    };
  }
}