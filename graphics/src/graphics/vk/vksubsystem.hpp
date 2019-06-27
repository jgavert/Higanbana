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
        , VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME
        , VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME
        , VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME
        , VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME
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
        "VK_KHR_external_memory_win32",
        "VK_KHR_external_semaphore_win32",
        "VK_KHR_external_fence_win32",
        "VK_KHR_swapchain",
        "VK_KHR_external_memory_win32",
        "VK_KHR_external_semaphore_win32",
        "VK_KHR_external_fence_win32",
        "VK_KHR_sampler_mirror_clamp_to_edge",
        "VK_KHR_push_descriptor",
        "VK_KHR_8bit_storage",
        "VK_EXT_shader_subgroup_ballot",
        "VK_EXT_shader_subgroup_vote",
        "VK_KHR_variable_pointers",
        "VK_EXT_sampler_filter_minmax",
        "VK_EXT_post_depth_coverage",
        "VK_EXT_shader_viewport_index_layer",
        "VK_EXT_shader_stencil_export",
        "VK_EXT_conservative_rasterization",
        "VK_EXT_sample_locations",
        "VK_KHR_draw_indirect_count",
        "VK_KHR_image_format_list",
        "VK_EXT_vertex_attribute_divisor",
        "VK_EXT_descriptor_indexing",
        "VK_EXT_inline_uniform_block",
        "VK_KHR_create_renderpass2",
        "VK_KHR_swapchain_mutable_format",
        "VK_KHR_driver_properties",
        "VK_KHR_vulkan_memory_model",
        "VK_EXT_conditional_rendering"
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