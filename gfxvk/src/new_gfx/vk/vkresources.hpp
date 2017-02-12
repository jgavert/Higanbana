#pragma once

#include "gfxvk/src/new_gfx/common/resources.hpp"

#include "gfxvk/src/new_gfx/vk/util/AllocStuff.hpp" // refactor
#include "gfxvk/src/new_gfx/vk/util/ShaderStorage.hpp"

#include <vulkan/vulkan.hpp>

namespace faze
{
  namespace backend
  {
    class HeapImpl
    {
    private:
      vk::DeviceMemory m_resource;

    public:
      HeapImpl(vk::DeviceMemory memory)
        : m_resource(memory)
      {}

      vk::DeviceMemory native()
      {
        return m_resource;
      }
    };

    class DeviceImpl
    {
    private:
      vk::Device                  m_device;
      vk::PhysicalDevice		      m_physDevice;
      bool                        m_debugLayer;
      std::vector<vk::QueueFamilyProperties> m_queues;
      bool                        m_singleQueue;
      bool                        m_computeQueues;
      bool                        m_dmaQueues;
      bool                        m_graphicQueues;
      vk::Queue                   m_internalUniversalQueue;
      GpuInfo                     m_info;
      ShaderStorage               m_shaders;
      int64_t                     m_resourceID = 1;

      struct FreeQueues
      {
        int universalIndex;
        int graphicsIndex;
        int computeIndex;
        int dmaIndex;
        std::vector<uint32_t> universal;
        std::vector<uint32_t> graphics;
        std::vector<uint32_t> compute;
        std::vector<uint32_t> dma;
      } m_freeQueueIndexes;

      struct MemoryTypes
      {
        int deviceLocalIndex = -1;
        int hostNormalIndex = -1;
        int hostCachedIndex = -1; 
        int deviceHostIndex = -1;
      } m_memoryTypes;

    public:
      DeviceImpl(
        vk::Device device,
        vk::PhysicalDevice physDev,
        FileSystem& fs,
        std::vector<vk::QueueFamilyProperties> queues,
        vk::PhysicalDeviceMemoryProperties memProp,
        GpuInfo info,
        bool debugLayer);
    };

    class SubsystemImpl
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
    public:
      SubsystemImpl(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
      GpuDevice createGpuDevice(FileSystem& fs, int id);
    };

  }
}