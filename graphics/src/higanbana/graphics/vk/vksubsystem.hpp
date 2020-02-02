#pragma once
#include "vkresources.hpp"

namespace higanbana
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
      bool m_debug;

      std::vector<std::string> devExtOrder = {
        "VK_KHR_8bit_storage",
        "VK_KHR_create_renderpass2",
        "VK_KHR_depth_stencil_resolve",
        "VK_KHR_draw_indirect_count",
        "VK_KHR_driver_properties",
        "VK_KHR_external_fence_win32",
        "VK_KHR_external_memory_win32",
        "VK_KHR_external_semaphore_win32",
        "VK_KHR_pipeline_executable_properties",
        "VK_KHR_image_format_list",
        "VK_KHR_push_descriptor",
        "VK_KHR_sampler_mirror_clamp_to_edge",
        "VK_KHR_shader_atomic_int64",
        "VK_KHR_shader_float16_int8",
        "VK_KHR_shader_float_controls",
        "VK_KHR_swapchain",
        "VK_KHR_swapchain_mutable_format",
        "VK_KHR_vulkan_memory_model",
        "VK_KHR_win32_keyed_mutex",
        "VK_EXT_blend_operation_advanced",
        "VK_EXT_buffer_device_address",
        "VK_EXT_conditional_rendering",
        "VK_EXT_conservative_rasterization",
        "VK_EXT_depth_clip_enable",
        "VK_EXT_depth_range_unrestricted",
        "VK_EXT_descriptor_indexing",
        "VK_EXT_discard_rectangles",
        "VK_EXT_external_memory_host",
        "VK_EXT_hdr_metadata",
        "VK_EXT_host_query_reset",
        "VK_EXT_inline_uniform_block",
        "VK_EXT_memory_budget",
        "VK_EXT_memory_priority",
        "VK_EXT_pci_bus_info",
        "VK_EXT_post_depth_coverage",
        "VK_EXT_sample_locations",
        "VK_EXT_sampler_filter_minmax",
        "VK_EXT_scalar_block_layout",
        "VK_EXT_shader_subgroup_ballot",
        "VK_EXT_shader_subgroup_vote",
        "VK_EXT_shader_viewport_index_layer",
        "VK_EXT_transform_feedback",
        "VK_EXT_vertex_attribute_divisor",
        "VK_KHR_timeline_semaphore",
        "VK_NV_mesh_shader"
      };

      std::vector<const char*> dev1_1Exts = {
        "VK_KHR_16bit_storage",
        "VK_KHR_bind_memory2",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_descriptor_update_template",
        "VK_KHR_device_group",
        "VK_KHR_external_fence",
        "VK_KHR_external_memory",
        "VK_KHR_external_semaphore",
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_maintenance1",
        "VK_KHR_maintenance2",
        "VK_KHR_maintenance3",
        "VK_KHR_multiview",
        "VK_KHR_relaxed_block_layout",
        //"VK_KHR_sampler_ycbcr_conversion",
        "VK_KHR_shader_draw_parameters",
        "VK_KHR_storage_buffer_storage_class",
        "VK_KHR_variable_pointers",
      };
    public:
      VulkanSubsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion, bool debug = false);
      std::string gfxApi() override;
      vector<GpuInfo> availableGpus(VendorID vendor) override;
      std::shared_ptr<backend::prototypes::DeviceImpl> createGpuDevice(FileSystem& fs, GpuInfo gpu) override;
      GraphicsSurface createSurface(Window& window) override;
    };
  }
}